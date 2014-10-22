#include "pch.h"
#include "main.h"
#include "services/service_manager.h"
#include "services/render_service.h"
#include "services/input_service.h"
#include "services/tracker_service.h"
#include <unicode/uclean.h>

#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#else
#include <sys/stat.h>
#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif
#endif

#ifdef __ANDROID__
#include <android_native_app_glue.h>
extern struct android_app* __state;
#endif



DfgGame::DfgGame()
    : _hyperKeyPressed(false)
{
}

void DfgGame::initialize()
{
	// create services

    _inputService = ServiceManager::Instance( ).RegisterService< InputService >( NULL );

    Service * render_dep[] = { _inputService, NULL };
	_renderService = ServiceManager::Instance( ).RegisterService< RenderService >( render_dep );

	setVsync( false );
    
    _userFolder = Game::getAppPrivateFolderPath( );
#ifdef WIN32
    _mkdir( _userFolder.c_str( ) );
#else
    mkdir( _userFolder.c_str( ), 0777 );
#endif
    _userFolder += "/Dream Farm Games";
#ifdef WIN32
    _mkdir( _userFolder.c_str( ) );
#else
    mkdir( _userFolder.c_str( ), 0777 );
#endif
    _userFolder += "/";
    _userFolder += getConfig( )->getNamespace( "window", true )->getString( "title" );
#ifdef WIN32
    _mkdir( _userFolder.c_str( ) );
#else
    mkdir( _userFolder.c_str( ), 0777 );
#endif

    // set default resources locale
    _gameLocale = getConfig( )->getString( "language" );

    // get system-wide locale and override _gameLocale if we have resources for it
    std::string newLocale;
#ifdef __APPLE__
    NSString* preferredLang = [[[NSBundle mainBundle] localizedInfoDictionary] objectForKey:@"GameLanguage"];
    newLocale = [preferredLang cStringUsingEncoding:NSASCIIStringEncoding];
#endif

#ifdef __ANDROID__
    android_app* app = __state;
    JNIEnv* env = app->activity->env;
    JavaVM* vm = app->activity->vm;
    vm->AttachCurrentThread(&env, NULL);

    jclass android_content_Context = env->FindClass("android/content/Context");
    jmethodID midGetPackageName = env->GetMethodID(android_content_Context, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring) env->CallObjectMethod(app->activity->clazz, midGetPackageName);
    jmethodID midGetResources = env->GetMethodID(android_content_Context, "getResources", "()Landroid/content/res/Resources;");
    jobject jResource = env->CallObjectMethod(app->activity->clazz, midGetResources);

    jclass resource_Class=env->GetObjectClass(jResource);
    jmethodID midGetIdentifier = env->GetMethodID(resource_Class, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
    jstring languageVar = env->NewStringUTF("GameLanguage");
    jstring TypeName = env->NewStringUTF("string");
    int id = env->CallObjectMethod(jResource, midGetIdentifier, languageVar, TypeName, packageName);

    jmethodID midGetAppName = env->GetMethodID(resource_Class, "getString", "(I)Ljava/lang/String;");
    jstring language = env->CallObjectMethod(jResource, midGetAppName, id);

    const char * lng = env->GetStringUTFChars(language, NULL);
    if (lng)
        newLocale = lng;
    env->ReleaseStringUTFChars(language, lng);

    vm->DetachCurrentThread();
#endif

    std::string aliasesName( "aliases_" );
    if( getConfig( )->getNamespace( ( aliasesName + newLocale ).c_str( ), true ) )
        _gameLocale = newLocale;

    gameplay::FileSystem::loadResourceAliases( getConfig( )->getNamespace( ( aliasesName + _gameLocale ).c_str( ), true ) );
}

void DfgGame::finalize()
{
	ServiceManager::Instance( ).Shutdown( );
    Caches::Instance( ).FlushAll( );

    u_cleanup( );
}

void DfgGame::update(float elapsedTime)
{
	ServiceManager::Instance( ).Update( elapsedTime );
}

void DfgGame::render(float /*elapsedTime*/)
{
	if( ServiceManager::Instance( ).GetState( ) == Service::ES_RUNNING )
		_renderService->RenderFrame( );
}

void DfgGame::keyEvent(gameplay::Keyboard::KeyEvent evt, int key)
{
    _inputService->InjectKeyEvent( evt, key );
    
    if( key == gameplay::Keyboard::KEY_HYPER )
        _hyperKeyPressed = evt == gameplay::Keyboard::KEY_PRESS;
    else if( _hyperKeyPressed && evt == gameplay::Keyboard::KEY_PRESS &&
            ( key == gameplay::Keyboard::KEY_CAPITAL_Q || key == gameplay::Keyboard::KEY_Q ) )
    {
        exit();
    }
}

void DfgGame::touchEvent(gameplay::Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    _inputService->InjectTouchEvent( evt, x, y, contactIndex );
}

bool DfgGame::mouseEvent(gameplay::Mouse::MouseEvent evt, int x, int y, int wheelDelta )
{
    return _inputService->InjectMouseEvent( evt, x, y, wheelDelta );
}

void DfgGame::gesturePinchEvent(int x, int y, float scale, int numberOfTouches)
{
    _inputService->InjectGesturePinchEvent(x, y, scale, numberOfTouches);
}

void DfgGame::pause()
{
    ServiceManager::Instance( ).signals.pauseEvent( );

    TrackerService * trackerService = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( trackerService )
        trackerService->EndSession();
    
    gameplay::Game::pause();
}

void DfgGame::resume()
{
    gameplay::Game::resume();

    ServiceManager::Instance( ).signals.resumeEvent( );
}

void DfgGame::reportError( bool isFatal, const char * errorMessage, ... )
{
    va_list args;
    va_start(args, errorMessage);

    char exceptionDesc[ 150 ];
    vsnprintf( exceptionDesc, 149, errorMessage, args );
    exceptionDesc[ 149 ] = '\0';

    if( isFatal )
    {
        // notify user
#ifdef WIN32
        MessageBoxA( NULL, exceptionDesc, "Critical Error", MB_OK | MB_ICONERROR );
#endif
    }

    TrackerService * trackerService = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( !trackerService )
        return;

    trackerService->SendException( exceptionDesc, isFatal );

    if( isFatal )
        trackerService->EndSession( );
}

void DfgGame::ScheduleLocalNotification( time_t datetime, const char * utf8Body, const char * utf8ActionButton, int badgeNumber, const std::unordered_map< std::string, std::string >& userDictionary )
{
#ifdef __APPLE__
#if TARGET_OS_IPHONE
    UILocalNotification *localNotif = [[UILocalNotification alloc] init];
    if (localNotif == nil)
        return;

    localNotif.fireDate = [NSDate dateWithTimeIntervalSince1970:datetime];
    localNotif.timeZone = [NSTimeZone defaultTimeZone];
 
    localNotif.alertBody = [NSString stringWithUTF8String:utf8Body];
    localNotif.alertAction = [NSString stringWithUTF8String:utf8ActionButton];
 
    localNotif.soundName = UILocalNotificationDefaultSoundName;
    localNotif.applicationIconBadgeNumber = badgeNumber;
 
    NSMutableDictionary *infoDict = [NSMutableDictionary dictionaryWithCapacity:userDictionary.size( )];

    for( std::unordered_map< std::string, std::string >::const_iterator it = userDictionary.begin( ), end_it = userDictionary.end( ); it != end_it; it++ )
        [infoDict setObject:[NSString stringWithUTF8String:( *it ).second.c_str( )] forKey:[NSString stringWithUTF8String:( *it ).first.c_str( )]];

    localNotif.userInfo = infoDict;
 
    [[UIApplication sharedApplication] scheduleLocalNotification:localNotif];
#endif
#endif
}

void DfgGame::CancelAllLocalNotifications( )
{
#ifdef __APPLE__
#if TARGET_OS_IPHONE
    [[UIApplication sharedApplication] cancelAllLocalNotifications];
    [UIApplication sharedApplication].applicationIconBadgeNumber = 0;
#endif
#endif
}