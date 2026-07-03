#include "SDK/foobar2000.h"

#import "DmsBrowserViewController.h"

#import "DmsTreeNode.h"

#include <memory>
#include <string>
#include <vector>

#include "DmsBrowseSession.hpp"
#include "DmsConfig.hpp"
#include "DmsPlaylistIntegration.h"
#include "upnp/UpnpError.hpp"

namespace {

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

} // namespace

@interface DmsBrowserViewController () <NSOutlineViewDataSource,
                                        NSOutlineViewDelegate, NSMenuDelegate>
@end

@implementation DmsBrowserViewController {
    NSPopUpButton* _serverPopup;
    NSOutlineView* _outlineView;
    NSTextField* _statusLabel;
    DmsTreeNode* _rootNode; // nil until a server is selected

    std::shared_ptr<dms::BrowseSession> _session;
    std::vector<component::ServerEntry> _servers;
    NSInteger _generation; // bumped on server switch; stale results dropped
}

- (void)loadView {
    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 680, 460)];

    _serverPopup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    _serverPopup.target = self;
    _serverPopup.action = @selector(onServerSelected:);
    _serverPopup.menu.delegate = self;

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

    NSStackView* topBar = [NSStackView stackViewWithViews:@[ _serverPopup ]];
    topBar.orientation = NSUserInterfaceLayoutOrientationHorizontal;

    for (NSView* subview in @[ topBar, scrollView, _statusLabel ]) {
        subview.translatesAutoresizingMaskIntoConstraints = NO;
        [view addSubview:subview];
    }
    [NSLayoutConstraint activateConstraints:@[
        [topBar.topAnchor constraintEqualToAnchor:view.topAnchor constant:8],
        [topBar.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:8],
        [topBar.trailingAnchor constraintLessThanOrEqualToAnchor:view.trailingAnchor
                                                        constant:-8],
        [scrollView.topAnchor constraintEqualToAnchor:topBar.bottomAnchor constant:8],
        [scrollView.leadingAnchor constraintEqualToAnchor:view.leadingAnchor],
        [scrollView.trailingAnchor constraintEqualToAnchor:view.trailingAnchor],
        [_statusLabel.topAnchor constraintEqualToAnchor:scrollView.bottomAnchor
                                               constant:4],
        [_statusLabel.leadingAnchor constraintEqualToAnchor:view.leadingAnchor
                                                   constant:8],
        [_statusLabel.trailingAnchor constraintEqualToAnchor:view.trailingAnchor
                                                    constant:-8],
        [_statusLabel.bottomAnchor constraintEqualToAnchor:view.bottomAnchor
                                                  constant:-6],
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
    ++_generation;
    NSString* name = [NSString stringWithUTF8String:server.name.c_str()];
    if (name.length == 0)
        name = [NSString stringWithUTF8String:server.url.c_str()] ?: @"Server";
    _rootNode = [DmsTreeNode rootNodeNamed:name];
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
        _statusLabel.stringValue = [NSString
            stringWithFormat:@"「%@」%lu 個項目", node.title,
                             (unsigned long)result.objects.size()];
    }
    [_outlineView reloadItem:node reloadChildren:YES];
    if (errorText == nil) [_outlineView expandItem:node];
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
            parent.state = DmsNodeStateIdle;
            [self loadChildrenOfNode:parent];
        }
    }
    // Container double-click: the default expand/collapse toggle applies.
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
        NSMenuItem* add = [menu addItemWithTitle:@"將曲目加入到目前播放清單"
                                          action:@selector(onAddClicked:)
                                   keyEquivalent:@""];
        add.target = self;
        add.enabled = node.state == DmsNodeStateLoaded;
        NSMenuItem* reload = [menu addItemWithTitle:@"重新載入"
                                             action:@selector(onReloadClicked:)
                                      keyEquivalent:@""];
        reload.target = self;
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
    // Container: its direct child items, single level (ADR-016).
    std::vector<upnp::UpnpObject> objects;
    for (DmsTreeNode* child in node.children) {
        if (child.kind == DmsNodeKindItem) objects.push_back(child.object);
    }
    [self addObjects:std::move(objects) sourceTitle:node.title];
}

- (void)onReloadClicked:(id)sender {
    DmsTreeNode* node = [self nodeAtRow:_outlineView.clickedRow];
    if (node == nil || ![node isContainer]) return;
    node.state = DmsNodeStateIdle;
    [self loadChildrenOfNode:node];
}

- (void)addObjects:(std::vector<upnp::UpnpObject>)objects
       sourceTitle:(NSString*)title {
    const size_t added = dms::addToActivePlaylist(objects);
    if (added == 0) {
        _statusLabel.stringValue = [NSString
            stringWithFormat:@"「%@」沒有可播放的項目", title];
        return;
    }
    _statusLabel.stringValue =
        [NSString stringWithFormat:@"已加入 %zu 首到目前播放清單", added];
}

- (DmsTreeNode*)nodeAtRow:(NSInteger)row {
    if (row < 0) return nil;
    return [_outlineView itemAtRow:row];
}

@end
