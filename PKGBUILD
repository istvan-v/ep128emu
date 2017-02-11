pkgname=ep128emu
pkgver=2.0.11.1
pkgrel=1
pkgdesc="Enterprise 64/128, ZX Spectrum 48/128, Amstrad CPC 464/664/6128 and Videoton TVC emulator"
url="https://github.com/istvan-v/ep128emu/"
arch=('i686' 'x86_64')
license=('GPL')
depends=('pulseaudio' 'curl' 'portaudio')
optdepends=('lua')
makedepends=('scons' 'lua' 'curl' 'jack' 'fltk' 'libjpeg-turbo' 'portaudio' 'sdl' 'libsndfile' 'libxft' 'glu')
source=("https://github.com/istvan-v/ep128emu/archive/${pkgver}.tar.gz")
md5sums=('D2A4692C91EE0054B981192B78FFFC6F')

build() {
	cd $srcdir/$pkgname-$pkgver/
	scons
}

package() {
	export UB_INSTALLDIR=$pkgdir
	cd $srcdir/$pkgname-$pkgver/
	scons install
}

