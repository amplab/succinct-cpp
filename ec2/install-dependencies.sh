# Must run scripts logged in as root
yum -y update

yum -y groupinstall "Development Tools"

yum -y install cmake gcc-c++ wget libevent-devel zlib-devel openssl-devel

mkdir tmpdir
cd tmpdir

echo "Installing autoconf 2.69..."
wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar xvf autoconf-2.69.tar.gz
cd autoconf-2.69
./configure --prefix=/usr
make
make install
cd ..

echo "Installing automake 1.14..."
wget http://ftp.gnu.org/gnu/automake/automake-1.14.tar.gz
tar xvf automake-1.14.tar.gz
cd automake-1.14
./configure --prefix=/usr
make
make install
cd ..

echo "Installing bison 2.5.1..."
wget http://ftp.gnu.org/gnu/bison/bison-2.5.1.tar.gz
tar xvf bison-2.5.1.tar.gz
cd bison-2.5.1
./configure --prefix=/usr
make
make install
cd ..

echo "Installing boost 1.55.0"
wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz
tar xvf boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh
./b2 install
cd ..

echo "Installing thrift (latest)"
git clone https://git-wip-us.apache.org/repos/asf/thrift.git
cd thrift
./bootstrap.sh
./configure --with-lua=no
make
make install
cd ..

cd ..
rm -rf tmpdir
