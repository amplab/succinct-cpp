gen=thrift-gen.files
rm -rf $gen
mkdir -p $gen/coordinator
mkdir -p $gen/server
mkdir -p $gen/utils
thrift -gen cpp -out $gen/coordinator coordinator.thrift
thrift -gen cpp -out $gen/server server.thrift
thrift -gen cpp -out $gen/utils heartbeat.thrift
rm -rf $gen/*/*skeleton*

cd $gen/coordinator
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..

cd $gen/server
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..

cd $gen/utils
rm heartbeat_constants.*
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
