rm -rf gen
mkdir -p gen/master
mkdir -p gen/handler
mkdir -p gen/server
mkdir -p gen/utils
thrift -gen cpp -out gen/master master.thrift
thrift -gen cpp -out gen/handler handler.thrift
thrift -gen cpp -out gen/server server.thrift
thrift -gen cpp -out gen/utils heartbeat.thrift
rm -rf gen/*/*skeleton*

cd gen/master
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..

cd gen/handler
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..

cd gen/server
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..

cd gen/utils
rm heartbeat_constants.*
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
