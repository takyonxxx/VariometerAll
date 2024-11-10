#import <CoreLocation/CoreLocation.h>

@interface LocationPermissionDelegate : NSObject <CLLocationManagerDelegate>
{
    CLLocationManager* locationManager;
}
@end

@implementation LocationPermissionDelegate

- (id)init
{
    self = [super init];
    if (self) {
        locationManager = [[CLLocationManager alloc] init];
        locationManager.delegate = self;
    }
    return self;
}

- (void)requestPermissions
{
    if ([CLLocationManager locationServicesEnabled]) {
        [locationManager requestWhenInUseAuthorization];
    }
}

- (void)locationManager:(CLLocationManager *)manager
    didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    switch (status) {
        case kCLAuthorizationStatusAuthorizedWhenInUse:
            NSLog(@"Permission granted");
            break;
        case kCLAuthorizationStatusDenied:
            NSLog(@"Permission denied");
            break;
        case kCLAuthorizationStatusNotDetermined:
            NSLog(@"Permission not determined yet");
            break;
        default:
            break;
    }
}
@end

static LocationPermissionDelegate* permissionDelegate = nil;

extern "C" void requestIOSLocationPermission() {
    if (!permissionDelegate) {
        permissionDelegate = [[LocationPermissionDelegate alloc] init];
    }
    [permissionDelegate requestPermissions];
}
