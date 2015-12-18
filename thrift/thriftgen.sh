mkdir -p gen/master
mkdir -p gen/handler
mkdir -p gen/server
thrift -gen cpp -out gen/master master.thrift
thrift -gen cpp -out gen/handler handler.thrift
thrift -gen cpp -out gen/server server.thrift
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
