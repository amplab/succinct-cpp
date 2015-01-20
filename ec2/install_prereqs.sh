# install prereqs
yum -y install libevent
yum -y install libevent-devel
yum -y install python-devel
yum -y install openssl-devel
yum -y install mysql-devel ruby-devel rubygems
yum -y install boost-devel
yum -y install flex
yum -y install bison
yum -y install gcc-c++

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
