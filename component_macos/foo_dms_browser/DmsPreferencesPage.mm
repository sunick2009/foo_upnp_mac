// Preferences → Tools → DMS Browser: manage the manual server list
// (ADR-015). Built programmatically — no xib (ADR-014).
#include "SDK/foobar2000.h"

#import <Cocoa/Cocoa.h>

#include <string>
#include <vector>

#include "DmsConfig.hpp"

@interface DmsPreferencesViewController
    : NSViewController <NSTableViewDataSource, NSTableViewDelegate,
                        NSTextFieldDelegate>
@end

@implementation DmsPreferencesViewController {
    NSTableView* _tableView;
    NSSegmentedControl* _addRemove;
    std::vector<component::ServerEntry> _servers;
}

- (void)loadView {
    _servers = dms::loadServers();

    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 560, 360)];

    _tableView = [[NSTableView alloc] initWithFrame:NSZeroRect];
    NSTableColumn* nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"名稱";
    nameColumn.width = 150;
    NSTableColumn* urlColumn = [[NSTableColumn alloc] initWithIdentifier:@"url"];
    urlColumn.title = @"Device description URL（rootDesc.xml）";
    urlColumn.width = 330;
    [_tableView addTableColumn:nameColumn];
    [_tableView addTableColumn:urlColumn];
    _tableView.dataSource = self;
    _tableView.delegate = self;
    _tableView.usesAlternatingRowBackgroundColors = YES;

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    scrollView.documentView = _tableView;
    scrollView.hasVerticalScroller = YES;
    scrollView.borderType = NSBezelBorder;

    _addRemove = [NSSegmentedControl
        segmentedControlWithImages:@[
            [NSImage imageNamed:NSImageNameAddTemplate],
            [NSImage imageNamed:NSImageNameRemoveTemplate]
        ]
                      trackingMode:NSSegmentSwitchTrackingMomentary
                            target:self
                            action:@selector(onAddRemove:)];

    NSTextField* hint = [NSTextField wrappingLabelWithString:
        @"新增 UPnP/DLNA Media Server 的 device description URL，"
        @"例如 http://192.168.1.10:8200/rootDesc.xml。"
        @"瀏覽視窗：主選單 View → DMS Browser。"];
    hint.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    hint.textColor = [NSColor secondaryLabelColor];

    for (NSView* subview in @[ scrollView, _addRemove, hint ]) {
        subview.translatesAutoresizingMaskIntoConstraints = NO;
        [view addSubview:subview];
    }
    [NSLayoutConstraint activateConstraints:@[
        [scrollView.topAnchor constraintEqualToAnchor:view.topAnchor constant:12],
        [scrollView.leadingAnchor constraintEqualToAnchor:view.leadingAnchor
                                                 constant:12],
        [scrollView.trailingAnchor constraintEqualToAnchor:view.trailingAnchor
                                                  constant:-12],
        // The scroll view must absorb all spare height, or an empty
        // table collapses and the buttons ride up into it.
        [scrollView.heightAnchor constraintGreaterThanOrEqualToConstant:150],
        [_addRemove.topAnchor constraintEqualToAnchor:scrollView.bottomAnchor
                                             constant:8],
        [_addRemove.leadingAnchor constraintEqualToAnchor:view.leadingAnchor
                                                 constant:12],
        [hint.topAnchor constraintEqualToAnchor:_addRemove.bottomAnchor constant:8],
        [hint.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:12],
        [hint.trailingAnchor constraintEqualToAnchor:view.trailingAnchor
                                            constant:-12],
        [hint.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:-12],
    ]];
    self.view = view;
}

- (void)persist {
    dms::saveServers(_servers);
}

- (void)viewWillDisappear {
    [super viewWillDisappear];
    // Closing Preferences (or switching pages) while a cell is still in
    // edit mode never fires controlTextDidEndEditing, so the typed value
    // is dropped — a fresh row then persists as just "http://". Resign
    // the field editor to commit the pending edit before teardown.
    NSWindow* window = self.view.window;
    if (window != nil && ![window makeFirstResponder:nil]) {
        [window endEditingFor:nil];
    }
}

- (void)onAddRemove:(NSSegmentedControl*)sender {
    if (sender.selectedSegment == 0) {
        _servers.push_back({"新伺服器", "http://"});
        [_tableView reloadData];
        [self persist];
        const NSInteger row = (NSInteger)_servers.size() - 1;
        [_tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row]
                byExtendingSelection:NO];
        [_tableView editColumn:0 row:row withEvent:nil select:YES];
    } else {
        const NSInteger row = _tableView.selectedRow;
        if (row < 0 || row >= (NSInteger)_servers.size()) return;
        _servers.erase(_servers.begin() + row);
        [_tableView reloadData];
        [self persist];
    }
}

#pragma mark - NSTableViewDataSource / Delegate

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    return (NSInteger)_servers.size();
}

- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)column
                   row:(NSInteger)row {
    NSTableCellView* cell = [tableView makeViewWithIdentifier:column.identifier
                                                        owner:self];
    if (cell == nil) {
        cell = [[NSTableCellView alloc] init];
        cell.identifier = column.identifier;
        NSTextField* text = [[NSTextField alloc] init];
        text.bordered = NO;
        text.drawsBackground = NO;
        text.editable = YES;
        // Delegate, not target/action: action fires only on Enter, so
        // clicking away would silently drop the edit (the cause of the
        // "component browses a stale URL" bug).
        text.delegate = self;
        text.translatesAutoresizingMaskIntoConstraints = NO;
        text.lineBreakMode = NSLineBreakByTruncatingTail;
        [cell addSubview:text];
        cell.textField = text;
        [NSLayoutConstraint activateConstraints:@[
            [text.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:2],
            [text.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor],
            [text.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
        ]];
    }
    const auto& server = _servers[(size_t)row];
    const bool isName = [column.identifier isEqualToString:@"name"];
    cell.textField.stringValue =
        [NSString stringWithUTF8String:(isName ? server.name : server.url).c_str()]
            ?: @"";
    return cell;
}

- (void)controlTextDidEndEditing:(NSNotification*)notification {
    NSTextField* sender = notification.object;
    const NSInteger row = [_tableView rowForView:sender];
    const NSInteger column = [_tableView columnForView:sender];
    if (row < 0 || row >= (NSInteger)_servers.size() || column < 0) return;
    auto& server = _servers[(size_t)row];
    const std::string value =
        component::trimWhitespace(sender.stringValue.UTF8String ?: "");
    if (column == 0) server.name = value;
    else server.url = value;
    sender.stringValue = [NSString stringWithUTF8String:value.c_str()] ?: @"";
    [self persist];
}

@end

namespace {

// {DFD4A132-8E50-41D7-94E0-3E8CF440E5F9}
const GUID kPreferencesPageGuid = {0xdfd4a132, 0x8e50, 0x41d7,
    {0x94, 0xe0, 0x3e, 0x8c, 0xf4, 0x40, 0xe5, 0xf9}};

class DmsPreferencesPage : public preferences_page {
public:
    service_ptr instantiate() override {
        return fb2k::wrapNSObject([DmsPreferencesViewController new]);
    }
    const char* get_name() override { return "DMS Browser"; }
    GUID get_guid() override { return kPreferencesPageGuid; }
    GUID get_parent_guid() override { return guid_tools; }
};

FB2K_SERVICE_FACTORY(DmsPreferencesPage);

} // namespace
