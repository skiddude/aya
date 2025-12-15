let
    nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-25.05";
    pkgs = import nixpkgs { config = {}; overlays = []; };
    bgfx = pkgs.clangStdenv.mkDerivation {
      name = "bgfx";
      src = pkgs.fetchFromGitHub {
        owner = "bkaradzic";
        repo = "bgfx.cmake";
        rev = "5f3f4f29726dbfa5e0de2a05e9daea1a89700c8d";
	hash = "sha256-O2+nSK74aXWTQ5QSBTqSeYBo4ikg/TTHzAeg6OYDJVM=";
	fetchSubmodules = true;
      };
      nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
      buildInputs = [ pkgs.xorg.libX11 pkgs.libGL pkgs.wayland pkgs.spirv-tools pkgs.spirv-cross pkgs.spirv-headers ];
      configurePhase = ''
        mkdir -p build
        cd build
        cmake ../ -DCMAKE_INSTALL_PREFIX=$out/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -GNinja
      '';
      buildPhase = ''
        cmake --build . --parallel $(nproc) --config Release
      '';
    };
    
    mkEnvironment = pkgs: name: pkgs.llvmPackages_latest.stdenv.mkDerivation {
        name = name;
        src = null;
        nativeBuildInputs = [ pkgs.llvmPackages_latest.clang-tools pkgs.cmake pkgs.pkg-config pkgs.ninja pkgs.gdb pkgs.bun pkgs.nodejs ];
        buildInputs = [
            bgfx
          
            pkgs.assimp
            pkgs.boost
            pkgs.curlFull.dev
            pkgs.libjpeg
            pkgs.freetype
            pkgs.openssl
            pkgs.libpng
            pkgs.lz4
            pkgs.SDL3
            pkgs.zlib
            pkgs.zstd
            pkgs.pugixml
            pkgs.imgui
            pkgs.discord-rpc
            pkgs.rapidjson
            pkgs.libjxl
            pkgs.cgal
            pkgs.gmp
            pkgs.mpfr
            pkgs.libarchive
            pkgs.kdePackages.qtbase
            pkgs.kdePackages.qttools
            pkgs.kdePackages.qtwebview
            pkgs.kdePackages.qtwebengine
            pkgs.kdePackages.qtdeclarative
            pkgs.kdePackages.full
            pkgs.xorg.libX11
            pkgs.libGLU
            pkgs.kdePackages.qtwayland
            pkgs.cef-binary
        ];
        CEF_ROOT = "${pkgs.cef-binary.out}/";
    };
in
{
    native = mkEnvironment pkgs "aya";
}
