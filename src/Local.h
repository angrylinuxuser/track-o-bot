#include <QScopedPointer>

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#define DEFINE_SINGLETON(CLASSNAME) \
public: \
  ~CLASSNAME( void ); \
  static CLASSNAME* Instance( void ) { \
    if( !_singletonptr ) \
      _singletonptr.reset(new CLASSNAME()); \
    return _singletonptr.data(); \
  } \
  static void Reset() { \
    _singletonptr.reset(); \
  } \
private: \
  static QScopedPointer<CLASSNAME> _singletonptr; \
  CLASSNAME( void ); \
  CLASSNAME( const CLASSNAME& ); \

#define DEFINE_SINGLETON_SCOPE( CLASSNAME ) \
  QScopedPointer<CLASSNAME> CLASSNAME::_singletonptr;

#include "Logger.h"

#define LOG(str, ...) Logger::Instance()->Add(LOG_INFO, str, ##__VA_ARGS__)
#define ERR(str, ...) Logger::Instance()->Add(LOG_ERROR, str, ##__VA_ARGS__)

#ifdef _DEBUG
#define DBG(str, ...) Logger::Instance()->Add(LOG_DEBUG, str, ##__VA_ARGS__)
#else
#define DBG(str, ...)
#endif

#define UNUSED_ARG(x) (void)(x)

const char *qt2cstr( const QString& str );
