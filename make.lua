local lm = require "luamake"

lm:build 'install' {
    '$luamake', 'lua', 'make/install.lua', lm.plat, '$luamake',
    pool = 'console'
}
