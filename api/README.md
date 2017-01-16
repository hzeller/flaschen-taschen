API
===

The FlaschenTaschen have a fairly simple [network protocol](../doc/protocols.md),
which you can implement in any programming language easily.
To make life simple, these are ready client APIs that you can use to get
started quickly.

Check out the [API examples](../examples-api-use) for usage examples.

If you have an implementation in a new language, consider contributing it here.

## Using the API in your project

If you want to use the API in your own project, it is probably best to use
it in a submodule.

Start by adding flaschen-taschen as sub-module. Here, we add it under the
subdirectory `ft`:

```
git submodule add https://github.com/hzeller/flaschen-taschen.git ft
```

(Read more about how to use [submodules in git][git-submodules])

This will check out the repository in the subdirectory `ft/`. The api
will be accessible in `ft/api`.

To users of your programs you can suggest to use `--recursive` when
they clone your project from git, so that they also automatically get the
submodules checked out.

## C++
If you are using C++, let's hook that in your toplevel Makefile:

```
FLASCHEN_TASCHEN_API_DIR=ft/api

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a
YOUR_OBJECTS=...

your-binary : $(YOUR_OBJECTS) $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(FTLIB) :
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib
```

See the [../examples-api-use/Makefile](../examples-api-use/Makefile) to get
inspired.

## Python
(TODO(Scotty): describe how to use the Python library in the `ft/api`
subdirectory)

[git-submodules]: http://git-scm.com/book/en/Git-Tools-Submodules
