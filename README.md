# lppm

`lppm` (lifelessPixels' Project Maker) is a very simple project template manager.

## Rationale

Author of this software (lifelessPixels) has a serious medical condition named
"perfectionism". Basically, it means that for every 10 started projects, maybe
one or two get to the point of relative usability. This tool is a tribute to that
fact - the author can now create new projects (that will never be finished) faster
and more easily than ever. The fact, that this software is working (kinda?) and
was finished is some kind of divine miracle, that the author cannot understand.

> [!NOTE]
> Take preceding comment with a GENEROUS grain of salt ;)

## Functional description

> [!WARNING]
> Documentation is being actively worked on!

## CLI reference

> [!WARNING]
> Documentation is being actively worked on!

## License

The project is licensed under permissive MIT license. More info can be found in [LICENSE](LICENSE)
file.

## Contributing

If you have any suggestions for the project, feel free to create pull requests/forks/etc.
I cannot guarantee that the changes will be merged into the mainline, but every help
will be really appreciated!

If you're going to submit a pull request, make sure that the code is formatted according
to [.clang-format](.clang-format) file contained in this repository!

## Known problems / roadmap

- in dire need of proper error propagation and handling (`result<T>` type instead
  of `std::optional<T>` and `std::variant<T, U>` types
- move common functionality in [handlers.cpp](src/handlers.cpp) file to separate
  functions to avoid code duplication
