/* Author: Zachary Drew
* Purpose: A dummy thread object to inform Cocoa that it needs to be thread safe.
*/

#import <Foundation/Foundation.h>

@interface DummyThread : NSObject
- init;
- (void)startThread:(id)arg;
@end

@implementation DummyThread
- init
{
	[super init];
	return self;
}
- (void)startThread:(id)arg
{
	return;
}
@end
