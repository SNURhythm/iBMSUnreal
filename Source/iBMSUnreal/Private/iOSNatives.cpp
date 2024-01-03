#if PLATFORM_IOS
#include "iOSNatives.h"

FString GetIOSDocumentsPath()
{
    return FString([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] UTF8String]);
}

// register touch event
void RegisterTouchEvent()
{
    // get root view controller
    UIViewController* rootViewController = [UIApplication sharedApplication].keyWindow.rootViewController;
    // get view
    UIView* view = rootViewController.view;
    // get touch event
    UITapGestureRecognizer* tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:rootViewController action:@selector(handleTapGesture:)];
    // add touch event
    [view addGestureRecognizer:tapGesture];
}
#endif