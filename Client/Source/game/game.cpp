#include "pch.h"
#include "game.h"
#include "services/render_service.h"
#include "services/tracker_service.h"
#include "services/debug_service.h"
#include "services/social_service.h"
#include "services/storefront_service.h"
#include "services/service_manager.h"
#include "services/ui_service.h"
#include "services/scene_service.h"
#include "render/particle_system.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#if TARGET_OS_IPHONE
#import "iRate/iRate.h"

@interface iRateTrackingDelegate : NSObject <iRateDelegate>
@end

@implementation iRateTrackingDelegate

- (void)iRateDidPromptForRating
{
    TrackerService * tracker = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( tracker )
        tracker->SendEvent("Rating", "Prompt", "");
}

- (void)iRateUserDidAttemptToRateApp
{
    TrackerService * tracker = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( tracker )
        tracker->SendEvent("Rating", "AttemptToRate", "");
}

- (void)iRateUserDidDeclineToRateApp
{
    TrackerService * tracker = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( tracker )
        tracker->SendEvent("Rating", "Decline", "");
}

- (void)iRateUserDidRequestReminderToRateApp
{
    TrackerService * tracker = ServiceManager::Instance( ).FindService< TrackerService >( );
    if( tracker )
        tracker->SendEvent("Rating", "Remind", "");
}

@end

#else

bool validateReceipt( )
{
    return true;
}

#endif
#endif


#ifdef __ANDROID__
const char * androidMainActivityClassName = "ru.vsu.mol.db";
#endif



// Declare our game instance
VSUDatabale game;


VSUDatabale::VSUDatabale()
    : _sceneService(nullptr)
{    
#ifdef _DEBUG
    
#if _MSC_VER
   _CrtSetDbgFlag ( _CRTDBG_CHECK_ALWAYS_DF | _crtDbgFlag );
   //_CrtSetBreakAlloc(7789);
#endif

    time_t seconds;
    time( &seconds );
    tm * t = localtime( &seconds );

    char outputFile[1024];
    sprintf( outputFile, "output_%02d-%02d-%02d_%02d-%02d-%02d.txt", t->tm_mon + 1, t->tm_mday, t->tm_year % 100, t->tm_hour, t->tm_min, t->tm_sec );
    char errorsFile[1024];
    sprintf( errorsFile, "errors_%02d-%02d-%02d_%02d-%02d-%02d.txt", t->tm_mon + 1, t->tm_mday, t->tm_year % 100, t->tm_hour, t->tm_min, t->tm_sec );

    freopen( outputFile, "w", stdout );
    freopen( errorsFile, "w", stderr );
#endif

#ifdef __APPLE__
#if TARGET_OS_IPHONE
    [iRate sharedInstance].daysUntilPrompt = 4;
    [iRate sharedInstance].usesUntilPrompt = 10;
    [iRate sharedInstance].promptAtLaunch = false;
    //[iRate sharedInstance].previewMode = true;
    [iRate sharedInstance].delegate = [[iRateTrackingDelegate alloc] init];
#endif
#endif
}

VSUDatabale::~VSUDatabale()
{
}

void VSUDatabale::initialize()
{
    DfgGame::initialize( );

#ifndef __ANDROID__
    std::string resPath( gameplay::FileSystem::getResourcePath() );
    gameplay::FileSystem::setResourcePath( ( resPath + "data/" ).c_str( ) );
#endif

    _clientUUID = GenerateUUID();

    loadSettings();
    //TrackerService * tracker = ServiceManager::Instance().RegisterService< TrackerService >(NULL);
    //tracker->SetTrackerEnabled(true);
    //tracker->SetupTracking("UA-53460880-1", _clientUUID.c_str(), "Loading");
    saveSettings();

    try
    {
        setVsync( false );
        setMultiTouch( true );

        Service * uiDep[] = { ServiceManager::Instance().FindService<RenderService>(), NULL };
        UIService * uiService = ServiceManager::Instance().RegisterService<UIService>(uiDep);

        Service * sceneDep[] = { uiService, NULL };
        _sceneService = ServiceManager::Instance().RegisterService<SceneService>(sceneDep);

#ifdef _DEBUG
        Service * debugDep[] = { uiService, NULL };
        DebugService * debugService = ServiceManager::Instance().RegisterService< DebugService >(debugDep);
        debugService->Setup( 
            _renderService->RegisterSpriteMaterial( MaterialAsset::Cache( ).Register( "materials/system/white_alpha.material" )->ShareAsset( ) ), 
            FontAsset::Cache( ).Register( "@font/arial.gpb" )->ShareAsset( ), 0.5f );
#endif
    }
    catch( std::exception& e )
    {
        //tracker->SendException( e.what( ), true );
        //tracker->EndSession( );
        throw e;
    }

    gameplay::Properties * properties = gameplay::Properties::create("@dictionary.txt");
    gameDictionary.Create(properties);
    SAFE_DELETE(properties);
}

void VSUDatabale::finalize()
{
    DfgGame::finalize( );
}

void VSUDatabale::render(float elapsedTime)
{
    PROFILE( "SODGame::render", "Render" );

    clear( gameplay::Game::CLEAR_COLOR_DEPTH, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0 );

    if (_sceneService)
        _sceneService->render();

    DfgGame::render( elapsedTime );
}

void VSUDatabale::loadSettings()
{
    std::string filename = std::string(GetUserDataFolder()) + "/settings.dat";
    try
    {
        gameplay::Stream * stream(gameplay::FileSystem::open(filename.c_str(), gameplay::FileSystem::READ));
        if (stream)
        {
            int32_t version = 0;
            if (stream->read(&version, sizeof(version), 1) != 1)
                throw "invalid settings file";

            char uuid[37];
            if (stream->read(&uuid, 36, 1) != 1)
                throw "invalid settings file";

            uuid[36] = '\0';
            _clientUUID = uuid;

            SAFE_DELETE(stream);
        }
    }
    catch (const char *)
    {
        // save settings file just to not lose any information
        // this file can later be processed by future versions of the app
        rename(filename.c_str(), (filename + "_error").c_str());
        saveSettings();
    }
}

void VSUDatabale::saveSettings()
{
    std::string filename = std::string(GetUserDataFolder()) + "/settings.dat";
    gameplay::Stream * stream(gameplay::FileSystem::open(filename.c_str(), gameplay::FileSystem::WRITE));

    if (stream)
    {
        GP_ASSERT(_clientUUID.size() == 36);

        int32_t version = 1;
        stream->write(&version, sizeof(version), 1);
        stream->write(_clientUUID.c_str(), 36, 1);

        SAFE_DELETE(stream);
    }
}