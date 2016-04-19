gen=thrift-gen.files
cp $gen/coordinator/*.cc ../coordinator/src/
cp $gen/coordinator/*.h ../coordinator/include/
cp $gen/server/*.cc ../server/src/
cp $gen/server/*.h ../server/include/
cp $gen/utils/*.cc ../utils/src/
cp $gen/utils/*.h ../utils/include/
