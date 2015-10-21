#ifndef GP_GLOBAL_H
#define GP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(GP_LIBRARY)
#  define GPSHARED_EXPORT Q_DECL_EXPORT
#else
#  define GPSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // GP_GLOBAL_H
