################################################################################
#
#common event provider
#
################################################################################

XMINERD_VERSION = 1.0.0
XMINERD_SOURCE = 
XMINERD_SITE  = 

XMINERD_LICENSE = 
XMINERD_LICENSE_FILES = README

XMINERD_MAINTAINED = YES
XMINERD_AUTORECONF = YES
XMINERD_INSTALL_STAGING = YES
XMINERD_DEPENDENCIES = jansson qlibminer

$(eval $(autotools-package))

