-- Boot/Bootstrap.lua
-- Only load once
if BootstrapLoaded then return end

System.Print("> Bootstrap Initializing ... ")
BootstrapLoaded = true

-- For local variables
Bootstrap = {}

-- Language modifications
require("Engine/Boot/Lang/Namespace")
require("Engine/Boot/Lang/String")
require("Engine/Boot/Lang/Meta")
require("Engine/Boot/Lang/Print")
require("Engine/Boot/Lang/Iterator")
require("Engine/Boot/Lang/Console")

-- Typed support
require("Engine/Boot/Typed/Loader")

-- Module modifications
require("Engine/Boot/Module/Runtime")

Bootstrap = nil
System.Print("> Bootstrap Initialized ... ")