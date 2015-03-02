# install prereqs
yum -y install libtool
yum -y install libevent
yum -y install libevent-devel
yum -y install python-devel
yum -y install openssl-devel
yum -y install mysql-devel ruby-devel rubygems
yum -y install boost-devel
yum -y install flex

mkdir -p /local/devtools

# Install Autoconf
yum -y install m4
cd /local/devtools
curl -OL http://ftpmirror.gnu.org/autoconf/autoconf-2.69.tar.gz
tar xzf autoconf-2.69.tar.gz
cd autoconf-2.69
./configure --prefix=/usr/local
make
sudo make install
export PATH=/usr/local/bin:$PATH

# Install Automake
cd /local/devtools
curl -OL http://ftpmirror.gnu.org/automake/automake-1.14.tar.gz
tar xzf automake-1.14.tar.gz
cd automake-1.14
./configure --prefix=/usr/local
make
sudo make install

# Install bison
cd /local/devtools
wget http://ftp.gnu.org/gnu/bison/bison-2.5.1.tar.gz
tar xvf bison-2.5.1.tar.gz
cd bison-2.5.1
./configure --prefix=/usr/local
make
sudo make install
