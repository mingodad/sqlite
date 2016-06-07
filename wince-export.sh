#! /bin/sh

export MINGW32CE_PATH=$HOME/local/opt/mingw32ce
export WINCE_PATH=$HOME/local/wince

export PATH=$MINGW32CE_PATH/bin:$PATH
export CPPFLAGS="-I$WINCE_PATH/include"
export LDFLAGS="-L$WINCE_PATH/lib"
export LD_LIBRARY_PATH="$WINCE_PATH/bin"
export PKG_CONFIG_PATH="$WINCE_PATH/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="$WINCE_PATH/lib/pkgconfig"