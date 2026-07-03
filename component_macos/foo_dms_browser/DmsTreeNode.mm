#import "DmsTreeNode.h"

@implementation DmsTreeNode {
    upnp::UpnpObject _object;
}

- (instancetype)initWithKind:(DmsNodeKind)kind
                       title:(NSString*)title
                      object:(upnp::UpnpObject)object {
    if ((self = [super init])) {
        _kind = kind;
        _title = [title copy];
        _object = std::move(object);
        _state = DmsNodeStateIdle;
        _children = [NSMutableArray array];
    }
    return self;
}

+ (instancetype)nodeWithObject:(const upnp::UpnpObject&)object {
    const BOOL container = object.type == upnp::UpnpObjectType::Container;
    NSString* title = [NSString stringWithUTF8String:object.title.c_str()]
        ?: @"(untitled)";
    return [[self alloc]
        initWithKind:(container ? DmsNodeKindContainer : DmsNodeKindItem)
               title:title
              object:object];
}

+ (instancetype)rootNodeNamed:(NSString*)name {
    upnp::UpnpObject root;
    root.type = upnp::UpnpObjectType::Container;
    root.id = "0";
    root.title = name.UTF8String ?: "Server";
    return [[self alloc] initWithKind:DmsNodeKindContainer
                                title:name
                               object:std::move(root)];
}

+ (instancetype)placeholderWithTitle:(NSString*)title {
    return [[self alloc] initWithKind:DmsNodeKindPlaceholder
                                title:title
                               object:{}];
}

- (upnp::UpnpObject)object {
    return _object;
}

- (BOOL)isContainer {
    return self.kind == DmsNodeKindContainer;
}

- (NSString*)objectId {
    return [NSString stringWithUTF8String:_object.id.c_str()] ?: @"";
}

@end
