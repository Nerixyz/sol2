.. image:: media/sol.png
	:target: https://github.com/nerixyz/sol2
	:alt: sol repository
	:align: center

sol2 (sol2, version |version|)
==============================
*a fast, simple C++ and Lua Binding*

When you need to hit the ground running with Lua and C++, `sol`_ is the go-to framework for high-performance binding with an easy to use API.

solpp
-----

`solpp <https://github.com/nerixyz/sol2>`_ is a fork that attempts to deliver some improvements over upstream.
As of December 2025, the last commit was nine months ago, yet there are bugs with active PRs containing fixes that aren't merged.

additional features
^^^^^^^^^^^^^^^^^^^

- Lua 5.5 support
- Luau support
- Added CI with more configurations (more compiler and Lua versions)
- Enabled and fixed the ``run_time`` tests.

Luau support
^^^^^^^^^^^^

Luau is in some aspects significantly different from Lua 5.1 (see `Differences from Lua <https://luau.org/compatibility/#differences-from-lua>`_).
These are the differences and important points for sol users:

- Vectors use ``sol::luau::vector``.
- Buffers use ``sol::luau::(const_)buffer_view``. To pass a buffer to Lua, use ``sol::copy_buffer``.
- If the target ``lua_State`` was not created by sol, you need to use ``sol::state_view::init_userdata_tags()`` on the state at the start.
- Currently, sol uses ``LUA_UTAG_LIMIT - 1`` as the tag for its usertypes. This is mainly due to a limitation of Luau to change the destructor for the inline userdata.
- Luau doesn't have a built-in ``require()`` implementation, hence, sol's APIs for this won't work.
- Most :doc:`api/policies` won't work, because ``lua_setfenv`` can't be used on userdata.
- Dumping function bytecode is unsupported.
- The ``lua_Reader`` APIs are unsupported.
- Due to the absence of ``__gc``, the destructors can't be customized with ``sol::meta_function::garbage_collect``.
- Once a usertype is defined in a state (``new_usertype``), it can't be removed, because the destructors can't modify the Luau state.

The following features are missing and would be nice to have:

- Ability to specify the userdata tag for certain types. This could also the type to be marked as non-dynamic.
- Userdata could use a table for ``__index`` if possible. Needs to be profiled, but Luau claims this is faster. An even faster method would be to use atoms.
- Userdata could support ``lua_UserdataDirectFieldGet```. Though it's tricky to detect when that's possible.
- Provide a sane implementation of ``require()`` out of the box.
- The bytecode produced from ``script()`` and friends can't be customized (e.g. optimization level).

get going:
----------

.. toctree::
	:maxdepth: 1
	:name: mastertoc
	
	tutorial/tutorial-top
	errors
	compilation
	features
	functions
	usertypes
	containers
	threading
	traits
	api/api-top
	mentions
	benchmarks
	performance
	safety
	exceptions
	rtti
	codecvt
	build
	licenses
	origin


connect
--------

Come to the Github Issues! We've got a friendly community, and they can help you out or you can come just to talk about the things you are working on!

|gh|




support
-------

You can support the project and other related endeavors in various ways.

|gs| |pa| |kf| |lp| |pp|  


This is a time-consuming effort, so individuals who donate get to:

- steer the direction and time spent on sol
- get their name put up in the CONTRIBUTORS list
- put something of their choice on sol2's README or the documentation's front page



"I need feature X, maybe you have it?"
--------------------------------------
Take a look at the :doc:`Features<features>` page: it links to much of the API. You can also just straight up browse the :doc:`api<api/api-top>` or ease in with the :doc:`tutorials<tutorial/tutorial-top>`. To know more about the implementation for usertypes, see :doc:`here<usertypes>` To know how function arguments are handled, see :ref:`this note<function-argument-handling>`. Don't see a feature you want? Send inquiries for support for a particular abstraction to the `issues`_ tracker.



the basics:
-----------

.. note::
	The code below *and* more examples can be found in the `examples directory`_.


.. literalinclude:: ../../examples/source/docs/simple_functions.cpp
	:name: simple-functions-example
	:linenos:

.. literalinclude:: ../../examples/source/docs/simple_structs.cpp
	:name: simple-structs-example
	:linenos:




Search
======

* :ref:`search`

.. _Sol: https://github.com/ThePhD/sol2
.. _issues: https://github.com/ThePhD/sol2/issues
.. _examples directory: https://github.com/ThePhD/sol2/tree/develop/examples

.. |pa| image:: media/become_a_patron_button.png
	:height: 50
	:target: https://www.patreon.com/soasis
	:alt: sol2 Patreon
	:align: middle
	
.. |kf| image:: media/Ko-fi_Blue.png
	:height: 50
	:target: https://ko-fi.com/soasis
	:alt: sol2 ko-fi
	:align: middle
	
.. |lp| image:: media/liberapay_logo.png
	:height: 50
	:target: https://liberapay.com/Soasis
	:alt: sol2 ko-fi
	:align: middle

.. |pp| image:: media/pp_cc_mark_111x69.jpg
	:height: 50
	:target: https://www.paypal.me/Soasis
	:alt: sol2 PayPal
	:align: middle

.. |gh| image:: media/github_logo.png
	:height: 75
	:target: https://github.com/ThePhD/sol2/issues
	:alt: sol2 Github Issues Page

.. |gs| image:: media/github_sponsors_logo.png
	:height: 55
	:target: https://github.com/users/ThePhD/sponsorship
	:alt: sol2 Sponsors Page

.. sol documentation master file, created by
   sphinx-quickstart on Mon Feb 29 21:49:51 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.
