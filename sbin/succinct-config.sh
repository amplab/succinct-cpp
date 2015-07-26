# resolve links - $0 may be a softlink
this="${BASH_SOURCE:-$0}"
common_bin="$(cd -P -- "$(dirname -- "$this")" && pwd -P)"
script="$(basename -- "$this")"
this="$common_bin/$script"

# convert relative path to absolute path
config_bin="`dirname "$this"`"
script="`basename "$this"`"
config_bin="`cd "$config_bin"; pwd`"
this="$config_bin/$script"

export SUCCINCT_PREFIX="`dirname "$this"`"/..
export SUCCINCT_HOME="${SUCCINCT_PREFIX}"
export SUCCINCT_CONF_DIR="${SUCCINCT_CONF_DIR:-"$SUCCINCT_HOME/conf"}"
export THRIFT_BIN_DIR="${THRIFT_BIN_DIR:-"$SUCCINCT_HOME/build/thrift/bin"}"
export ADAPTIVE_BIN_DIR="${ADAPTIVE_BIN_DIR:-"$SUCCINCT_HOME/build/adaptive/bin"}"
