#include "SDK/foobar2000.h"

#import "DmsBrowserViewController.h"

#import "DmsTreeNode.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "DmsBrowseSession.hpp"
#include "DmsConfig.hpp"
#include "DmsPlaylistIntegration.h"
#include "component/RecursiveBrowseCollector.hpp"
#include "upnp/ResourceSelector.hpp"
#include "upnp/UpnpError.hpp"

namespace {

constexpr size_t kMaxRecursiveTracks = 10000;
constexpr size_t kMaxRecursiveContainers = 10000;

// Single serial queue for all blocking network work (ADR-017): browses
// never run concurrently, so one HttpClient per session is safe.
dispatch_queue_t workerQueue() {
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        queue = dispatch_queue_create("org.foo-dms-browser.worker",
                                      DISPATCH_QUEUE_SERIAL);
    });
    return queue;
}

NSString* describeBrowseError() {
    try {
        throw; // rethrow current exception to classify it
    } catch (const upnp::SoapFaultException& e) {
        return [NSString stringWithFormat:@"伺服器拒絕（SOAP fault %d）：%s",
                                          e.faultCode, e.faultString.c_str()];
    } catch (const upnp::HttpException& e) {
        if (e.statusCode < 0)
            return [NSString stringWithFormat:@"連線失敗：%s", e.what()];
        return [NSString stringWithFormat:@"HTTP %d：%s", e.statusCode, e.what()];
    } catch (const upnp::XmlParseException& e) {
        // Typical cause: the URL answers but isn't a device description
        // (e.g. a router or web server returning an HTML page).
        return [NSString
            stringWithFormat:@"回應無法解析（此 URL 可能不是 UPnP device "
                             @"description）：%s", e.what()];
    } catch (const std::exception& e) {
        return [NSString stringWithFormat:@"%s", e.what()];
    } catch (...) {
        return @"未知錯誤";
    }
}

NSString* describePlaybackResource(const upnp::UpnpObject& object) {
    const auto resource = upnp::selectBestResource(object.resources);
    if (!resource) return @"無可播放 HTTP resource";

    NSMutableArray<NSString*>* parts = [NSMutableArray array];
    if (!resource->mimeType.empty()) {
        NSString* mime = [NSString stringWithUTF8String:resource->mimeType.c_str()];
        if (mime.length > 0) [parts addObject:mime];
    }
    if (!resource->duration.empty()) {
        NSString* duration =
            [NSString stringWithUTF8String:resource->duration.c_str()];
        if (duration.length > 0) [parts addObject:duration];
    }
    if (resource->sampleFrequency) {
        [parts addObject:[NSString stringWithFormat:@"%u Hz",
                          *resource->sampleFrequency]];
    }
    if (resource->bitsPerSample) {
        [parts addObject:[NSString stringWithFormat:@"%u-bit",
                          *resource->bitsPerSample]];
    }
    if (resource->nrAudioChannels) {
        [parts addObject:[NSString stringWithFormat:@"%u ch",
                          *resource->nrAudioChannels]];
    }
    return parts.count > 0 ? [parts componentsJoinedByString:@" / "]
                           : @"HTTP resource";
}

struct RecursiveAddJob {
    std::atomic_bool cancelled{false};
};

} // namespace

@interface DmsBrowserViewController () <NSOutlineViewDataSource,
                                        NSOutlineViewDelegate, NSMenuDelegate>
@end

@implementation DmsBrowserViewController {
    NSPopUpButton* _serverPopup;
    NSButton* _cancelAddButton;
    NSOutlineView* _outlineView;
    NSStackView* _detailBar;
    NSImageView* _albumArtView;
    NSTextField* _selectionMetaLabel;
    NSTextField* _statusLabel;
    DmsTreeNode* _rootNode; // nil until a server is selected

    std::shared_ptr<dms::BrowseSession> _session;
    std::shared_ptr<RecursiveAddJob> _recursiveAddJob;
    std::vector<component::ServerEntry> _servers;
    NSInteger _generation; // bumped on server switch; stale results dropped
    NSInteger _artGeneration;
}

- (void)loadView {
    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 680, 460)];

    _serverPopup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    _serverPopup.target = self;
    _serverPopup.action = @selector(onServerSelected:);
    _serverPopup.menu.delegate = self;

    _cancelAddButton = [NSButton buttonWithTitle:@"取消加入"
                                          target:self
                                          action:@selector(onCancelRecursiveAdd:)];
    _cancelAddButton.bezelStyle = NSBezelStyleRounded;
    _cancelAddButton.hidden = YES;

    _statusLabel = [NSTextField labelWithString:@""];
    _statusLabel.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    _statusLabel.textColor = [NSColor secondaryLabelColor];
    _statusLabel.lineBreakMode = NSLineBreakByTruncatingTail;

    _outlineView = [[NSOutlineView alloc] initWithFrame:NSZeroRect];
    NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"title"];
    column.title = @"標題";
    [_outlineView addTableColumn:column];
    _outlineView.outlineTableColumn = column;
    _outlineView.headerView = nil;
    _outlineView.dataSource = self;
    _outlineView.delegate = self;
    _outlineView.target = self;
    _outlineView.doubleAction = @selector(onDoubleClick:);

    NSMenu* contextMenu = [[NSMenu alloc] init];
    contextMenu.delegate = self;
    _outlineView.menu = contextMenu;

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    scrollView.documentView = _outlineView;
    scrollView.hasVerticalScroller = YES;

    _albumArtView = [[NSImageView alloc] initWithFrame:NSZeroRect];
    _albumArtView.imageScaling = NSImageScaleProportionallyUpOrDown;
    _albumArtView.imageAlignment = NSImageAlignCenter;
    _albumArtView.translatesAutoresizingMaskIntoConstraints = NO;
    [_albumArtView.widthAnchor constraintEqualToConstant:96].active = YES;
    [_albumArtView.heightAnchor constraintEqualToConstant:96].active = YES;

    _selectionMetaLabel = [NSTextField wrappingLabelWithString:@""];
    _selectionMetaLabel.font =
        [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    _selectionMetaLabel.textColor = [NSColor secondaryLabelColor];
    _selectionMetaLabel.lineBreakMode = NSLineBreakByTruncatingTail;

    _detailBar =
        [NSStackView stackViewWithViews:@[ _albumArtView, _selectionMetaLabel ]];
    _detailBar.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    _detailBar.alignment = NSLayoutAttributeTop;
    _detailBar.spacing = 8;
    _detailBar.hidden = YES;

    NSStackView* topBar =
        [NSStackView stackViewWithViews:@[ _serverPopup, _cancelAddButton ]];
    topBar.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    topBar.spacing = 8;

    NSStackView* mainStack =
        [NSStackView stackViewWithViews:@[ topBar, scrollView, _detailBar, _statusLabel ]];
    mainStack.orientation = NSUserInterfaceLayoutOrientationVertical;
    mainStack.spacing = 6;
    mainStack.edgeInsets = NSEdgeInsetsMake(8, 8, 6, 8);
    mainStack.translatesAutoresizingMaskIntoConstraints = NO;
    [view addSubview:mainStack];
    [NSLayoutConstraint activateConstraints:@[
        [mainStack.topAnchor constraintEqualToAnchor:view.topAnchor],
        [mainStack.leadingAnchor constraintEqualToAnchor:view.leadingAnchor],
        [mainStack.trailingAnchor constraintEqualToAnchor:view.trailingAnchor],
        [mainStack.bottomAnchor constraintEqualToAnchor:view.bottomAnchor],
        [scrollView.heightAnchor constraintGreaterThanOrEqualToConstant:180],
    ]];

    self.view = view;
    [self reloadServerListSelecting:0];
}

- (void)viewDidAppear {
    [super viewDidAppear];
    // Pick up Preferences edits whenever the user comes back to this
    // window — otherwise an already-open browser keeps browsing the
    // old URL after the server list was fixed.
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onWindowBecameKey:)
               name:NSWindowDidBecomeKeyNotification
             object:self.view.window];
}

- (void)viewDidDisappear {
    [super viewDidDisappear];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
                  name:NSWindowDidBecomeKeyNotification
                object:self.view.window];
}

- (void)onWindowBecameKey:(NSNotification*)notification {
    if (dms::loadServers() != _servers) {
        [self reloadServerListSelecting:[_serverPopup indexOfSelectedItem]];
    }
}

#pragma mark - Server list

- (void)reloadServerListSelecting:(NSInteger)index {
    _servers = dms::loadServers();
    [_serverPopup removeAllItems];
    for (const auto& server : _servers) {
        NSString* title = [NSString stringWithUTF8String:server.name.c_str()];
        if (title.length == 0)
            title = [NSString stringWithUTF8String:server.url.c_str()] ?: @"?";
        // NSPopUpButton dedupes equal titles; go through the menu directly.
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:nil
                                               keyEquivalent:@""];
        [_serverPopup.menu addItem:item];
    }
    if (_servers.empty()) {
        _statusLabel.stringValue =
            @"尚未設定伺服器 — Preferences → Tools → DMS Browser 新增";
        _rootNode = nil;
        _session.reset();
        [self clearSelectionDetails];
        ++_generation;
        [_outlineView reloadData];
        return;
    }
    if (index < 0 || index >= (NSInteger)_servers.size()) index = 0;
    [_serverPopup selectItemAtIndex:index];
    [self connectToServerAtIndex:index];
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
    if (menu == _outlineView.menu) {
        [self rebuildContextMenu:menu];
        return;
    }
    // Server popup about to open: pick up Preferences edits.
    const auto fresh = dms::loadServers();
    if (fresh != _servers) {
        [self reloadServerListSelecting:[_serverPopup indexOfSelectedItem]];
    }
}

- (void)onServerSelected:(id)sender {
    [self connectToServerAtIndex:[_serverPopup indexOfSelectedItem]];
}

- (void)connectToServerAtIndex:(NSInteger)index {
    if (index < 0 || index >= (NSInteger)_servers.size()) return;
    const auto& server = _servers[(size_t)index];
    _session = std::make_shared<dms::BrowseSession>(server.url);
    if (_recursiveAddJob) _recursiveAddJob->cancelled = true;
    _recursiveAddJob.reset();
    _cancelAddButton.hidden = YES;
    ++_generation;
    NSString* name = [NSString stringWithUTF8String:server.name.c_str()];
    if (name.length == 0)
        name = [NSString stringWithUTF8String:server.url.c_str()] ?: @"Server";
    _rootNode = [DmsTreeNode rootNodeNamed:name];
    [self clearSelectionDetails];
    [_outlineView reloadData];
    _statusLabel.stringValue = @"";
    [_outlineView expandItem:_rootNode];
}

#pragma mark - Loading

- (void)loadChildrenOfNode:(DmsTreeNode*)node {
    if (node.state == DmsNodeStateLoading || node.state == DmsNodeStateLoaded)
        return;
    node.state = DmsNodeStateLoading;
    node.children =
        [NSMutableArray arrayWithObject:[DmsTreeNode placeholderWithTitle:@"載入中…"]];
    [_outlineView reloadItem:node reloadChildren:YES];

    auto session = _session;
    const NSInteger generation = _generation;
    const std::string objectId = node.objectId.UTF8String ?: "0";

    dispatch_async(workerQueue(), ^{
        component::PagedBrowseResult result;
        NSString* errorText = nil;
        try {
            result = session->fetchChildren(objectId);
        } catch (...) {
            errorText = describeBrowseError();
            // Log the exact URL: a stray character in the configured
            // address is invisible in the UI but obvious here.
            FB2K_console_formatter()
                << "DMS Browser: browse objectId=\"" << objectId.c_str()
                << "\" failed, server=\"" << session->rootDescUrl().c_str()
                << "\": " << errorText.UTF8String;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            [self applyBrowseResult:std::move(result)
                             toNode:node
                              error:errorText
                         generation:generation];
        });
    });
}

- (void)applyBrowseResult:(component::PagedBrowseResult)result
                   toNode:(DmsTreeNode*)node
                    error:(NSString*)errorText
               generation:(NSInteger)generation {
    if (generation != _generation) return; // server switched meanwhile

    if (errorText != nil) {
        node.state = DmsNodeStateFailed;
        node.children = [NSMutableArray arrayWithObject:
            [DmsTreeNode placeholderWithTitle:
                [NSString stringWithFormat:@"⚠️ %@（雙擊此列重試）", errorText]]];
        _statusLabel.stringValue = errorText;
    } else {
        node.state = DmsNodeStateLoaded;
        NSMutableArray<DmsTreeNode*>* children = [NSMutableArray array];
        for (const auto& object : result.objects)
            [children addObject:[DmsTreeNode nodeWithObject:object]];
        if (result.truncated) {
            [children addObject:[DmsTreeNode placeholderWithTitle:
                @"⚠️ 內容過多，僅載入前 10,000 項"]];
        }
        node.children = children;
        // Prefix with 已載入: this line describes the last completed
        // load, and without it users read it as describing the current
        // selection once they click into another container.
        _statusLabel.stringValue = [NSString
            stringWithFormat:@"已載入「%@」：%lu 個項目", node.title,
                             (unsigned long)result.objects.size()];
    }
    [_outlineView reloadItem:node reloadChildren:YES];
    if (errorText == nil) [_outlineView expandItem:node];
}

#pragma mark - Album art / selection details

- (void)clearSelectionDetails {
    ++_artGeneration;
    _albumArtView.image = nil;
    _selectionMetaLabel.stringValue = @"";
    _detailBar.hidden = YES;
}

- (void)outlineViewSelectionDidChange:(NSNotification*)notification {
    (void)notification;
    DmsTreeNode* node = [self nodeAtRow:_outlineView.selectedRow];
    if (node == nil || node.kind != DmsNodeKindItem) {
        [self clearSelectionDetails];
        return;
    }

    const upnp::UpnpObject object = node.object;
    NSMutableArray<NSString*>* lines = [NSMutableArray array];
    [lines addObject:node.title ?: @""];
    NSString* artist = object.artist
        ? [NSString stringWithUTF8String:object.artist->c_str()]
        : nil;
    NSString* album = object.album
        ? [NSString stringWithUTF8String:object.album->c_str()]
        : nil;
    NSMutableArray<NSString*>* detailParts = [NSMutableArray array];
    if (artist.length > 0) [detailParts addObject:artist];
    if (album.length > 0) [detailParts addObject:album];
    if (object.date && !object.date->empty()) {
        NSString* date = [NSString stringWithUTF8String:object.date->c_str()];
        if (date.length > 0) [detailParts addObject:date];
    }
    if (detailParts.count > 0)
        [lines addObject:[detailParts componentsJoinedByString:@" / "]];
    [lines addObject:describePlaybackResource(object)];
    _selectionMetaLabel.stringValue = [lines componentsJoinedByString:@"\n"];
    _detailBar.hidden = NO;

    ++_artGeneration;
    const NSInteger artGeneration = _artGeneration;
    _albumArtView.image = nil;
    if (!object.albumArtUri || object.albumArtUri->empty()) return;

    NSString* uri = [NSString stringWithUTF8String:object.albumArtUri->c_str()];
    NSURL* url = [NSURL URLWithString:uri ?: @""];
    if (url == nil) return;

    NSURLSessionDataTask* task = [[NSURLSession sharedSession]
          dataTaskWithURL:url
        completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
            (void)response;
            if (error != nil || data.length == 0) return;
            NSImage* image = [[NSImage alloc] initWithData:data];
            if (image == nil) return;
            dispatch_async(dispatch_get_main_queue(), ^{
                if (artGeneration != _artGeneration) return;
                _albumArtView.image = image;
            });
        }];
    [task resume];
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView*)outlineView
    numberOfChildrenOfItem:(id)item {
    if (item == nil) return _rootNode ? 1 : 0;
    return (NSInteger)((DmsTreeNode*)item).children.count;
}

- (id)outlineView:(NSOutlineView*)outlineView
            child:(NSInteger)index
           ofItem:(id)item {
    if (item == nil) return _rootNode;
    return ((DmsTreeNode*)item).children[(NSUInteger)index];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item {
    return [(DmsTreeNode*)item isContainer];
}

#pragma mark - NSOutlineViewDelegate

- (NSView*)outlineView:(NSOutlineView*)outlineView
    viewForTableColumn:(NSTableColumn*)tableColumn
                  item:(id)item {
    DmsTreeNode* node = item;
    NSTableCellView* cell = [outlineView makeViewWithIdentifier:@"cell" owner:self];
    if (cell == nil) {
        cell = [[NSTableCellView alloc] init];
        cell.identifier = @"cell";
        NSTextField* text = [NSTextField labelWithString:@""];
        text.translatesAutoresizingMaskIntoConstraints = NO;
        text.lineBreakMode = NSLineBreakByTruncatingTail;
        [cell addSubview:text];
        cell.textField = text;
        [NSLayoutConstraint activateConstraints:@[
            [text.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor],
            [text.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor],
            [text.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
        ]];
    }
    cell.textField.stringValue = node.title ?: @"";
    cell.textField.textColor = node.kind == DmsNodeKindPlaceholder
        ? [NSColor secondaryLabelColor]
        : [NSColor labelColor];
    return cell;
}

- (void)outlineViewItemWillExpand:(NSNotification*)notification {
    DmsTreeNode* node = notification.userInfo[@"NSObject"];
    // Only untouched nodes load here. A Failed node must NOT auto-retry:
    // reloadItem re-fires willExpand on expanded items, so resetting
    // Failed here creates an infinite fail-retry loop that hammers the
    // server. Retry is explicit — double-click the error row or use the
    // context menu.
    if (node.state != DmsNodeStateIdle) return;
    [self loadChildrenOfNode:node];
}

#pragma mark - Actions

- (void)onDoubleClick:(id)sender {
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil) return;
    if (node.kind == DmsNodeKindItem) {
        std::vector<upnp::UpnpObject> single{node.object};
        [self addObjects:std::move(single) sourceTitle:node.title];
        return;
    }
    if (node.kind == DmsNodeKindPlaceholder) {
        // Error row: explicit retry of the failed parent.
        DmsTreeNode* parent = [_outlineView parentForItem:node];
        if (parent != nil && parent.state == DmsNodeStateFailed) {
            [self retryFailedNode:parent];
        }
        return;
    }
    if ([node isContainer] && node.state == DmsNodeStateFailed) {
        // Double-clicking the failed container row itself is as explicit
        // a gesture as its error child row; willExpand still refuses
        // Failed nodes, so this cannot re-enter the auto-retry loop.
        [self retryFailedNode:node];
        return;
    }
    // Container double-click: the default expand/collapse toggle applies.
}

- (void)retryFailedNode:(DmsTreeNode*)node {
    FB2K_console_formatter()
        << "DMS Browser: manual retry objectId=\""
        << (node.objectId.UTF8String ?: "0") << "\"";
    node.state = DmsNodeStateIdle;
    [self loadChildrenOfNode:node];
}

- (void)rebuildContextMenu:(NSMenu*)menu {
    [menu removeAllItems];
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil) return;
    if (node.kind == DmsNodeKindItem) {
        NSMenuItem* add = [menu addItemWithTitle:@"加入到目前播放清單"
                                          action:@selector(onAddClicked:)
                                   keyEquivalent:@""];
        add.target = self;
    } else if ([node isContainer]) {
        NSMenuItem* addDirect = [menu addItemWithTitle:@"加入直接子項曲目"
                                                action:@selector(onAddClicked:)
                                         keyEquivalent:@""];
        addDirect.target = self;
        // Loaded state is no longer required: unloaded containers fetch
        // their direct children on demand (issue #11).
        addDirect.enabled = !_recursiveAddJob;
        NSMenuItem* addRecursive = [menu addItemWithTitle:@"遞迴加入所有曲目"
                                                   action:@selector(onAddRecursiveClicked:)
                                            keyEquivalent:@""];
        addRecursive.target = self;
        addRecursive.enabled = !_recursiveAddJob;
        NSMenuItem* reload = [menu addItemWithTitle:@"重新載入"
                                             action:@selector(onReloadClicked:)
                                      keyEquivalent:@""];
        reload.target = self;
        reload.enabled = !_recursiveAddJob;
    }
}

- (void)onAddClicked:(id)sender {
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil) return;
    if (node.kind == DmsNodeKindItem) {
        std::vector<upnp::UpnpObject> single{node.object};
        [self addObjects:std::move(single) sourceTitle:node.title];
        return;
    }
    if (![node isContainer]) return;
    // Container: its direct child items, single level (ADR-016).
    if (node.state == DmsNodeStateLoaded) {
        std::vector<upnp::UpnpObject> objects;
        for (DmsTreeNode* child in node.children) {
            if (child.kind == DmsNodeKindItem) objects.push_back(child.object);
        }
        [self addObjects:std::move(objects) sourceTitle:node.title];
        return;
    }
    // Never-expanded container (issue #11): fetch its direct children on
    // demand instead of reading the not-yet-loaded tree.
    [self startDirectAddForNode:node];
}

- (void)startDirectAddForNode:(DmsTreeNode*)node {
    if (_session == nullptr || _recursiveAddJob != nullptr) return;
    auto session = _session;
    auto job = std::make_shared<RecursiveAddJob>();
    _recursiveAddJob = job;
    _cancelAddButton.hidden = NO;
    const NSInteger generation = _generation;
    const std::string objectId = node.objectId.UTF8String ?: "0";
    NSString* sourceTitle = [node.title copy] ?: @"";
    _statusLabel.stringValue = [NSString
        stringWithFormat:@"正在取得「%@」的直接子項曲目…", sourceTitle];

    dispatch_async(workerQueue(), ^{
        component::PagedBrowseResult page;
        NSString* errorText = nil;
        try {
            page = session->fetchChildren(objectId);
        } catch (...) {
            errorText = describeBrowseError();
            FB2K_console_formatter()
                << "DMS Browser: direct add objectId=\"" << objectId.c_str()
                << "\" failed, server=\"" << session->rootDescUrl().c_str()
                << "\": " << errorText.UTF8String;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            if (generation != _generation || _recursiveAddJob != job) return;
            _recursiveAddJob.reset();
            _cancelAddButton.hidden = YES;
            if (errorText != nil) {
                _statusLabel.stringValue = errorText;
                return;
            }
            if (job->cancelled.load()) {
                _statusLabel.stringValue = [NSString
                    stringWithFormat:@"已取消「%@」加入，未加入曲目", sourceTitle];
                return;
            }
            // addToActivePlaylist skips child containers itself and
            // counts unplayable items, so the status line matches the
            // expanded-container path.
            [self addObjects:std::move(page.objects) sourceTitle:sourceTitle];
        });
    });
}

- (void)onAddRecursiveClicked:(id)sender {
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil || ![node isContainer] || _session == nullptr ||
        _recursiveAddJob != nullptr)
        return;

    auto session = _session;
    auto job = std::make_shared<RecursiveAddJob>();
    _recursiveAddJob = job;
    _cancelAddButton.hidden = NO;

    const NSInteger generation = _generation;
    const std::string rootObjectId = node.objectId.UTF8String ?: "0";
    NSString* sourceTitle = [node.title copy] ?: @"";
    _statusLabel.stringValue =
        [NSString stringWithFormat:@"正在遞迴掃描「%@」…", sourceTitle];

    dispatch_async(workerQueue(), ^{
        component::RecursiveBrowseResult result;
        NSString* errorText = nil;
        try {
            result = component::collectRecursiveChildren(
                [session](const std::string& objectId) {
                    return session->fetchChildren(objectId);
                },
                rootObjectId,
                component::RecursiveBrowseOptions{
                    .maxTracks = kMaxRecursiveTracks,
                    .maxContainers = kMaxRecursiveContainers,
                },
                [job] { return job->cancelled.load(); },
                [=](component::RecursiveBrowseProgress progress) {
                    const size_t containersVisited = progress.containersVisited;
                    const size_t tracksFound = progress.tracksFound;
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (generation != _generation || _recursiveAddJob != job) return;
                        _statusLabel.stringValue = [NSString
                            stringWithFormat:
                                @"正在遞迴掃描「%@」：%zu 個資料夾，%zu 首",
                                sourceTitle, containersVisited, tracksFound];
                    });
                });
        } catch (...) {
            errorText = describeBrowseError();
            FB2K_console_formatter()
                << "DMS Browser: recursive add root objectId=\""
                << rootObjectId.c_str() << "\" failed, server=\""
                << session->rootDescUrl().c_str() << "\": "
                << errorText.UTF8String;
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            [self finishRecursiveAdd:std::move(result)
                                root:sourceTitle
                               error:errorText
                          generation:generation
                                 job:job];
        });
    });
}

- (void)onCancelRecursiveAdd:(id)sender {
    if (_recursiveAddJob) {
        _recursiveAddJob->cancelled = true;
        _statusLabel.stringValue = @"正在取消遞迴加入…";
    }
}

- (void)onReloadClicked:(id)sender {
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil || ![node isContainer]) return;
    node.state = DmsNodeStateIdle;
    [self loadChildrenOfNode:node];
}

- (void)finishRecursiveAdd:(component::RecursiveBrowseResult)result
                      root:(NSString*)rootTitle
                     error:(NSString*)errorText
                generation:(NSInteger)generation
                       job:(std::shared_ptr<RecursiveAddJob>)job {
    if (generation != _generation || _recursiveAddJob != job) return;
    _recursiveAddJob.reset();
    _cancelAddButton.hidden = YES;

    if (errorText != nil) {
        _statusLabel.stringValue = errorText;
        return;
    }
    if (result.cancelled) {
        _statusLabel.stringValue =
            [NSString stringWithFormat:@"已取消「%@」遞迴加入，未加入曲目", rootTitle];
        return;
    }

    const dms::AddToPlaylistResult added = dms::addToActivePlaylist(result.objects);
    if (added.added == 0) {
        _statusLabel.stringValue = [NSString
            stringWithFormat:@"「%@」沒有可播放的項目", rootTitle];
        return;
    }
    NSString* suffix = result.truncated ? @"（已達掃描上限）" : @"";
    NSString* skipped = added.skipped > 0
        ? [NSString stringWithFormat:@"，略過 %zu 個不可播放項目", added.skipped]
        : @"";
    // Fast scans overwrite the live progress line almost immediately, so
    // the completion line must carry the container count itself.
    _statusLabel.stringValue = [NSString
        stringWithFormat:@"已從「%@」遞迴加入 %zu 首（掃描 %zu 個資料夾）%@%@",
                         rootTitle, added.added, result.containersVisited,
                         skipped, suffix];
}

- (void)addObjects:(std::vector<upnp::UpnpObject>)objects
       sourceTitle:(NSString*)title {
    const dms::AddToPlaylistResult result = dms::addToActivePlaylist(objects);
    if (result.added == 0) {
        _statusLabel.stringValue = [NSString
            stringWithFormat:@"「%@」沒有可播放的項目", title];
        return;
    }
    NSString* skipped = result.skipped > 0
        ? [NSString stringWithFormat:@"，略過 %zu 個不可播放項目", result.skipped]
        : @"";
    _statusLabel.stringValue = [NSString
        stringWithFormat:@"已加入 %zu 首到目前播放清單%@", result.added, skipped];
}

- (DmsTreeNode*)nodeAtRow:(NSInteger)row {
    if (row < 0) return nil;
    return [_outlineView itemAtRow:row];
}

@end
