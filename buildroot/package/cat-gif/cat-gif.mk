################################################################################
#
# cat-gif
#
################################################################################
# Information: https://buildroot.org/downloads/manual/manual.html#cmake-package-tutorial

# CAT_GIF* prefix comes from the package name and the config in 'Config.in'

# Version string used in the build directory name and for downloads. For a
# "local" source it is only a label.
CAT_GIF_VERSION = 1.0

# Location of source code. Will rsync because of *_SITE_METHOD.
# BR2_EXTERNAL_CATGIF_PATH defined by Buildroot and points to the external tree
# this name is defined in external.desc
CAT_GIF_SITE = $(BR2_EXTERNAL_CATGIF_PATH)/..
CAT_GIF_SITE_METHOD = local

# Don't copy large or irrelevant directories into Buildroot's build tree: the
# desktop build output, the vendored SDL copies (Buildroot supplies its own),
# and local test GIFs.
CAT_GIF_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = \
	--exclude /build \
	--exclude /vendored \
	--exclude /gifs \
	--exclude /gifs.bak

# Packages that must be built and installed in the sysroot before this one.
# sdl3 and sdl3_image provide the headers and libraries we link against. This
# controls build order; Config.in controls what the user can select.
CAT_GIF_DEPENDENCIES = sdl3 sdl3_image

# Extra options passed to cmake, which will select which libs it's built against
# for outside of buildroot, it uses the vendored copies that are submodules.
CAT_GIF_CONF_OPTS = -DCATGIF_SYSTEM_SDL=ON

# Auto-start at boot (BusyBox init). Buildroot runs this hook only when the init
# system is sysv/busybox. CAT_GIF_PKGDIR is this package's directory.
define CAT_GIF_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(CAT_GIF_PKGDIR)/S99cat-gif \
		$(TARGET_DIR)/etc/init.d/S99cat-gif
endef

# Generate the build rules for a CMake-based target package. It configures with
# the cross toolchain file, builds, and runs "make install" into the target
# directory, so CMakeLists.txt needs an install() rule for the executable.
$(eval $(cmake-package))
