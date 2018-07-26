project "jx"
	kind "StaticLib"

	includedirs {
		path.join(BX_DIR, "include"),
		path.join(BX_DIR, "3rdparty"),
		path.join(JX_DIR, "include"),
		path.join(JTL_DIR, "include")
	}

	files {
		path.join(JX_DIR, "include/**.h"),
		path.join(JX_DIR, "include/**.inl"),
		path.join(JX_DIR, "src/**.cpp"),
		path.join(JX_DIR, "src/**.h"),
	}

	configuration { "Debug" }
		defines {
			"JX_CONFIG_DEBUG=1",
		}

	configuration { "vs*", "x64" }
		includedirs {
			path.join(BX_DIR,   "include/compat/msvc"),
		}
		
	configuration { "asmjs" }
		defines {
			"JX_CONFIG_MATH_SIMD=0",
		}
		
	configuration {}
