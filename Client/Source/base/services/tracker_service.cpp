#include "pch.h"
#include "tracker_service.h"
#include "service_manager.h"
#include "main.h"
#include "curl/curl.h"




bool TrackerService::_threadForceQuit = false;
int TrackerService::_dispatchPeriod = 1000;
int TrackerService::_dispatchRate = 1;
std::auto_ptr< std::thread > TrackerService::_dispatchThread;

TrackerService::TrackerService( const ServiceManager * manager )
    : Service( manager )
    , _trackingId( )
    , _clientId( )
    , _protocolVersion( 1 )
    , _curl( NULL )
    , _allowDispatch( true )
    , _sessionStartTime( 0 )
    , _lastPayloadSentTime( 0 )
{
    memset( _customMetrics, 0, sizeof( _customMetrics ) );
    time( &_trackerStartTime );
}

TrackerService::~TrackerService( )
{
}

static char errorBuffer[ CURL_ERROR_SIZE ];
void TrackerService::SetupTracking( const char * gaAppId, const char * clientId, const char * startScreen )
{
    _trackingId = gaAppId;
    _clientId = clientId;

    _appName = UrlEncode( gameplay::Game::getInstance( )->getConfig( )->getNamespace( "window", true )->getString( "title" ) );
    _appVersion = UrlEncode( gameplay::Game::getInstance( )->getConfig( )->getNamespace( "window", true )->getString( "version" ) );

    // load previously saved payloads which weren't sent to server
    std::string filename = std::string( static_cast< DfgGame * >( gameplay::Game::getInstance( ) )->GetUserDataFolder( ) ) + "/payloads.dat";
    gameplay::Stream * stream( gameplay::FileSystem::open( filename.c_str( ), gameplay::FileSystem::READ ) );

    if( stream )
    {
        int version = 0;
        stream->read( &version, sizeof( version ), 1 );

        if( version == 1 )
        {
            time_t lastTrackerStartTime;
            stream->read( &lastTrackerStartTime, sizeof( lastTrackerStartTime ), 1 );

            int32_t payloadsCount = 0;
            stream->read( &payloadsCount, sizeof( payloadsCount ), 1 );

            int payloadDiffInSeconds = static_cast< int >( difftime( _trackerStartTime, lastTrackerStartTime ) );

            for( int i = 0; i < payloadsCount; i++ )
            {
                int size = 0;

                stream->read( &size, sizeof( size ), 1 );

                char * str = reinterpret_cast< char * >( alloca( size + 1 ) );
                stream->read( str, size, 1 );
                str[ size ] = '\0';

                PayloadInfo payload;
                payload.params = str;

                stream->read( &payload.createTime, sizeof( payload.createTime ), 1 );

                // adjust payload's createTime by diff between runs
                payload.createTime -= static_cast< float >( payloadDiffInSeconds * 1000.0f );
                
                // filter out too old payloads (older thah 4h)
                if( payload.createTime > -4.0f * 3600.0f * 1000.0f )
                    _payloadsQueue.push_back( payload );
            }
        }

        SAFE_DELETE( stream );
    }

    _curl = curl_easy_init();
    if( _curl )
    {
        const char * endpoint = "http://www.google-analytics.com/collect";
        curl_easy_setopt( _curl, CURLOPT_URL, endpoint );
        curl_easy_setopt( _curl, CURLOPT_POST, 1 ); 
        curl_easy_setopt( _curl, CURLOPT_USERAGENT, gameplay::Game::getInstance( )->getUserAgentString( ) );
        //curl_easy_setopt( _curl, CURLOPT_DNS_CACHE_TIMEOUT, -1 );
        curl_easy_setopt( _curl, CURLOPT_NOSIGNAL, 1 );
        curl_easy_setopt( _curl, CURLOPT_TIMEOUT, 4 );
        curl_easy_setopt( _curl, CURLOPT_CONNECTTIMEOUT, 2 );
        curl_easy_setopt( _curl, CURLOPT_ERRORBUFFER, errorBuffer );
        curl_easy_setopt( _curl, CURLOPT_TCP_NODELAY, 1 );  // make sure packets are sent immediately
    }

    if( !_payloadsQueue.empty( ) )
    {
        // make sure previous session is ended
        EndSession( );
    }

    _currentView = UrlEncode( startScreen );
    SendData( "t=appview&cd=%s",
        _currentView.c_str( )
        );
}

bool TrackerService::OnInit( )
{
    _dispatchThread.reset( new std::thread( &TrackerService::DispatchThreadProc, this ) );
    return true;
}

bool TrackerService::OnShutdown( )
{
    // try to empty payloads queue, thread will exit right after sleep.
    EndSession( );

    if( _dispatchThread.get( ) )
    {
        _dispatchMutex.lock();
        _threadForceQuit = true;
        _dispatchMutex.unlock();
        _dispatchThread->join();
    }

    if( _curl )
        curl_easy_cleanup( _curl );
    
    _payloadsQueue.clear( );

    return true;
}

void TrackerService::FlushPayloads()
{
    std::string filename = std::string( static_cast< DfgGame * >( gameplay::Game::getInstance( ) )->GetUserDataFolder( ) ) + "/payloads.dat";
    gameplay::Stream * stream( gameplay::FileSystem::open( filename.c_str( ), gameplay::FileSystem::WRITE ) );
    
    if( stream )
    {
        int version = 1;
        stream->write( &version, sizeof( version ), 1 );
        stream->write( &_trackerStartTime, sizeof( _trackerStartTime ), 1 );
        
        int maxPayloadsToStore = std::min< int >( 1024, _payloadsQueue.size( ) );
        stream->write( &maxPayloadsToStore, sizeof( maxPayloadsToStore ), 1 );
        _payloadQueueMutex.lock( );
        std::deque< PayloadInfo >::iterator it = _payloadsQueue.begin( );
        while( maxPayloadsToStore-- > 0 )
        {
            GP_ASSERT( it != _payloadsQueue.end( ) );
            const PayloadInfo& payload = *it;
            
            int32_t size = static_cast< int32_t >(payload.params.size( ));
            stream->write( &size, sizeof( size ), 1 );
            stream->write( payload.params.c_str( ), size, 1 );
            stream->write( &payload.createTime, sizeof( payload.createTime ), 1 );

            it++;
        }
        _payloadQueueMutex.unlock( );
        
        SAFE_DELETE( stream );
    }
}

bool TrackerService::OnTick( )
{
    return false; // as long as service is active dispatch thread is working
}

void TrackerService::ForceDispatch( int dispatchCount )
{
    _dispatchMutex.lock( );

    // if thread is forced to quit, _payloadsQueue may be invalid
    if (_threadForceQuit)
    {
        _dispatchMutex.unlock();
        return;
    }

    bool flushAll = dispatchCount == 0;
    dispatchCount = std::min< int >( dispatchCount, static_cast< int >(_payloadsQueue.size( )) - 1 );
    while( flushAll && !_payloadsQueue.empty( ) || dispatchCount > 0 )
    {
        // get a copy of payload because queue can be modified in another thread
        PayloadInfo payload = _payloadsQueue.front( );

        if( !Dispatch( payload ) )
            break;

        _payloadQueueMutex.lock( );
        _payloadsQueue.pop_front( );
        _payloadQueueMutex.unlock( );

        dispatchCount--;
    }
    _dispatchMutex.unlock( );
}

void TrackerService::EndSession( )
{
    if( !_payloadsQueue.empty( ) && _payloadsQueue.back( ).params.find( "&sc=end" ) == _payloadsQueue.back( ).params.npos )
        _payloadsQueue.back( ).params += "&sc=end";

    ForceDispatch( 0 );

    _dispatchMutex.lock( );
    _sessionStartTime = 0;
    _dispatchMutex.unlock( );

    // if payload queue still not empty (e.g., internet connection was lost)
    // store it offline to send payloads on next run
    FlushPayloads();
}

bool TrackerService::Dispatch( const PayloadInfo& payload )
{
    if( !_allowDispatch )
        return false;

    static char finalRequest[ 4096 ];
    float curTime = static_cast< float >( gameplay::Game::getInstance( )->getAbsoluteTime( ) );

    int timeDiff = static_cast< int >( curTime - payload.createTime );
    sprintf( finalRequest, "v=%d&tid=%s&cid=%s&an=%s&av=%s&%s", 
        _protocolVersion, _trackingId.c_str( ), _clientId.c_str( ), 
        _appName.c_str( ),
        _appVersion.c_str( ),
        payload.params.c_str( ) );

    // reset session when time between payloads passed midnight
    time_t time1 = _trackerStartTime + static_cast< int >( _lastPayloadSentTime * 0.001f );
    time_t time2 = _trackerStartTime + static_cast< int >( curTime * 0.001f );
    int day1 = localtime( &time1 )->tm_mday;
    int day2 = localtime( &time2 )->tm_mday;

    if( payload.createTime >= 0 &&
        ( _sessionStartTime <= 0
        || ( payload.createTime - _sessionStartTime ) > 240.0f * 60.0f * 1000.0f     // sessions in GA are automaticaly ended after some period of time
        || day1 != day2 )
        )
    {
        // send language and resolution for first payload
        // assuming the first payload starts a session
        int width = gameplay::Game::getInstance( )->getWidth( );
        int height = gameplay::Game::getInstance( )->getHeight( );

        sprintf( finalRequest, "%s&sr=%dx%d&ul=%s&sc=start", 
            finalRequest,
            width, height, 
            static_cast< DfgGame * >( gameplay::Game::getInstance( ) )->GetGameLocale( )
            );
    }

    if( timeDiff > 0 )
        sprintf( finalRequest, "%s&qt=%d", 
            finalRequest,
            timeDiff
            );

    curl_easy_setopt( _curl, CURLOPT_POSTFIELDS, finalRequest );
    CURLcode res = curl_easy_perform( _curl );

    if( res == CURLE_OK )
    {
        if( _sessionStartTime <= 0 )
            _sessionStartTime = payload.createTime;

        _lastPayloadSentTime = payload.createTime;
    }

#ifdef _DEBUG
    time_t seconds;
    time( &seconds );
    tm * t = localtime( &seconds );

    gameplay::Logger::log( gameplay::Logger::LEVEL_INFO, 
        "%02d-%02d-%02d %02d:%02d:%02d %08d %08d GA: %s - %d\n", 
        t->tm_mon + 1, t->tm_mday, t->tm_year % 100, t->tm_hour, t->tm_min, t->tm_sec,
        static_cast< int >( curTime ), static_cast< int >( payload.createTime ), finalRequest, res );
    
    GP_ASSERT( abs( static_cast< int >( curTime ) - timeDiff - static_cast< int >( payload.createTime ) ) < 2 );
#endif

    return res == CURLE_OK;
}

void TrackerService::SendView( const char * screenName )
{
    if (_currentView == screenName)
        return;

    _currentView = UrlEncode( screenName );
    SendData( "t=appview&cd=%s", _currentView.c_str( ) );
}

void TrackerService::SendEvent( const char * category, const char * action, const char * label, const int& value, bool interactive )
{
    SendData( "t=event&ec=%s&ea=%s&el=%s&ev=%u&ni=%d&cd=%s",
        UrlEncode( category ),
        UrlEncode( action ),
        UrlEncode( label ),
        value,
        interactive ? 0 : 1,
        _currentView.c_str( )
        );
}

void TrackerService::SendSocialEvent( const char * network, const char * action, const char * target )
{
    SendData( "t=social&sn=%s&sa=%s&st=%s&cd=%s",
        UrlEncode( network ),
        UrlEncode( action ),
        UrlEncode( target ),
        _currentView.c_str( )
        );
}

void TrackerService::SendTiming( const char * category, const int& timeMs, const char * name, const char * label )
{
    SendData( "t=timing&utc=%s&utv=%s&utt=%d&utl=%s&cd=%s",
        UrlEncode( category ),
        name == NULL ? "" : UrlEncode( name ),
        timeMs,
        label == NULL ? "" : UrlEncode( label ),
        _currentView.c_str( )
        );
}

void TrackerService::SendException( const char * type, bool isFatal )
{
    SendData( "t=exception&exd=%s&exf=%d&cd=%s",
        UrlEncode( type ),
        isFatal ? 1 : 0,
        _currentView.c_str( )
        );
}

void TrackerService::SendTransaction( const char * transactionID, const char * store, float totalRevenue, float shippingCost, float tax, const char * currencyCode )
{
    SendData( "t=transaction&ti=%s&ta=%s&tr=%.2f&ts=%.2f&tt=%.2f&cu=%s",
        UrlEncode( transactionID ),
        UrlEncode( store ),
        totalRevenue,
        shippingCost,
        tax,
        UrlEncode( currencyCode )
        );
}

void TrackerService::SendItem( const char * transactionID, const char * name, float price, int quantity, const char * sku, const char * category, const char * currencyCode )
{
    SendData( "t=item&ti=%s&in=%s&ip=%.2f&iq=%d&ic=%s&iv=%s&cu=%s",
        UrlEncode( transactionID ),
        UrlEncode( name ),
        price,
        quantity,
        UrlEncode( sku ),
        UrlEncode( category ),
        UrlEncode( currencyCode )
        );
}

void TrackerService::SetCustomDimension( const int& id, const char * dimension )
{
    if( id < 0 || id > MAX_CUSTOM_DIMENSIONS )
        return;

    _customDimensions[ id ] = dimension;
}

void TrackerService::SetCustomMetric( const int& id, const int& metric )
{
    if( id < 0 || id > MAX_CUSTOM_METRICS )
        return;

    _customMetrics[ id ] = metric;
}

void TrackerService::SendData( const char * format, ... )
{
    va_list args;
    va_start(args, format);

    char request[ 4096 ];
    vsnprintf( request, 4095, format, args );
    request[ 4095 ] = '\0';

    int i;
    for( i = 0; i < MAX_CUSTOM_DIMENSIONS; i++ )
        if( !_customDimensions[ i ].empty( ) )
        {
            char buf[ 256 ];
            sprintf( buf, "&cd%d=%s", i + 1, UrlEncode( _customDimensions[ i ].c_str( ) ) );
            strncat( request, buf, 4095 );
        }

    for( i = 0; i < MAX_CUSTOM_METRICS; i++ )
        if( _customMetrics[ i ] > 0 )
        {
            char buf[ 256 ];
            sprintf( buf, "&cm%d=%d", i + 1, _customMetrics[ i ] );
            strncat( request, buf, 4095 );
        }

    PayloadInfo payload;
    payload.params = request;
    payload.createTime = static_cast< float >( gameplay::Game::getInstance( )->getAbsoluteTime( ) );
    
    _payloadQueueMutex.lock( );
    _payloadsQueue.push_back( payload );
    _payloadQueueMutex.unlock( );

    memset( _customMetrics, 0, sizeof( _customMetrics ) );
    for( i = 0; i < MAX_CUSTOM_DIMENSIONS; i++ )
        _customDimensions[ i ].clear( );
}

const char * TrackerService::UrlEncode( const char * str )
{
    static std::string res[16];
    static int resInd = 0;
    char * escapedStr = curl_easy_escape( _curl, str, static_cast< int >(strlen( str )) );
    res[resInd] = escapedStr;
    const char * resStr = res[resInd].c_str( );
    resInd = ( resInd + 1 ) & 15;

    curl_free( escapedStr );

    return resStr;
}

void TrackerService::DispatchThreadProc( void * arg )
{
    TrackerService * _this = reinterpret_cast< TrackerService * >( arg );

    if( !_this->_curl )
        return;

    while( !_this->_threadForceQuit && ( _this->GetState( ) == Service::ES_INITIALIZING || _this->GetState( ) == Service::ES_RUNNING ) )
    {
        _this->ForceDispatch( _this->_dispatchRate );
		std::this_thread::sleep_for(std::chrono::milliseconds(_this->_dispatchPeriod));
    }
}