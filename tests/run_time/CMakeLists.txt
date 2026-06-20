# # # # sol2
# The MIT License (MIT)
#
# Copyright (c) 2013-2022 Rapptz, ThePhD, and contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# # # # sol2 tests - runtime tests

file(GLOB sources
	LIST_DIRECTORIES FALSE
	CONFIGURE_DEPENDS
	source/*.cpp)

# Many tests are broken. Only include the working ones.
set(working_tests
	basic
	container_semantics
	container_semantics.custom
	container_semantics.ordered
	container_semantics.unordered
	container_shims
	# Broken because of detail::ebco
	# container_table
	container.usertype_check
	containers
	containers.roundtrip
	# Broken because of detail::ebco
	# containers.table
	coroutines
	customizations_private
	customizations
	dump
	# FIXME: Contains one SOL_BROKEN_TESTS
	functions
	functions.std
	gc
	lua_value
	operators
	overflow
	plain_types
	# At least one test is broken. Haven't looked deeper into fixing it.
	# policies
	proxies
	state
	storage
	strings
	tables.checks
	tables.clear
	tables
	tables.enums
	tables.indexing
	tables.insertion
	tables.traversal
	usertypes.auxiliary_keys
	usertypes.basic
	usertypes.constructors
	usertypes
	usertypes.inheritance
	usertypes.inheritance.multi
	usertypes.member_variables
	usertypes.overload
	# FIXME: Contains two SOL_BROKEN_TESTS
	usertypes.properties
	usertypes.runtime
	usertypes.unique
	# FIXME: Contains one SOL_BROKEN_TESTS
	usertypes.unregister
	utility
	variadics
)
list(JOIN working_tests | working_re)
list(FILTER sources INCLUDE REGEX "[/\\\\](main|${working_re})\\.cpp$")

list(LENGTH working_tests working_len)
list(LENGTH sources sources_len)
math(EXPR sources_len "${sources_len} - 1") # account for main.cpp
if (NOT working_len EQUAL sources_len)
	message(FATAL_ERROR "Mismatch between specified (#=${working_len}) and matched (#=${sources_len}) tests.")
endif()

sol2_create_basic_test(sol2.tests.run_time sol2::sol2 ${sources})
target_compile_definitions(sol2.tests.run_time PRIVATE
	SOL_ALL_SAFETIES_ON=1)
if (SOL2_TESTS_SINGLE)
	sol2_create_basic_test(sol2.single.tests.run_time sol2::sol2::single ${sources})
	target_compile_definitions(sol2.single.tests.run_time PRIVATE
		SOL_ALL_SAFETIES_ON=1)
endif()
