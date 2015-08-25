mkdir -p thrift
thrift -I include/thrift -gen cpp:include_prefix -out thrift succinct.thrift
rm -rf thrift/*skeleton*
cd thrift
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
