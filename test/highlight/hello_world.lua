local math = require("math")
local str = require("string")

_G.message = "Hello, world!"

function main(arg)
    print(arg)
    return 0
end

main(_G.message)