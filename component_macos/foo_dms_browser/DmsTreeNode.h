#pragma once

#import <Foundation/Foundation.h>

#include "upnp/UpnpObject.hpp"

typedef NS_ENUM(NSInteger, DmsNodeState) {
    DmsNodeStateIdle,
    DmsNodeStateLoading,
    DmsNodeStateLoaded,
    DmsNodeStateFailed,
};

typedef NS_ENUM(NSInteger, DmsNodeKind) {
    DmsNodeKindContainer,
    DmsNodeKindItem,
    DmsNodeKindPlaceholder, // "loading…" / error rows
};

// One row in the browser tree. UI state lives here; the DIDL payload
// is kept as a C++ value for playlist integration.
@interface DmsTreeNode : NSObject

@property(nonatomic, readonly) DmsNodeKind kind;
@property(nonatomic, readonly, strong) NSString* title;
@property(nonatomic, readonly) upnp::UpnpObject object; // empty for placeholders
@property(nonatomic) DmsNodeState state;                // containers only
@property(nonatomic, strong) NSMutableArray<DmsTreeNode*>* children;

+ (instancetype)nodeWithObject:(const upnp::UpnpObject&)object;
+ (instancetype)rootNodeNamed:(NSString*)name; // objectId "0" container
+ (instancetype)placeholderWithTitle:(NSString*)title;

- (BOOL)isContainer;
- (NSString*)objectId;

@end
