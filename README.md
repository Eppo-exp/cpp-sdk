# Eppo C++ SDK

[Eppo](https://geteppo.com) is a modular flagging and experimentation analysis tool.

## Building

```bash
make build
```

This will automatically generate `compile_commands.json` for IDE support if it doesn't already exist, and will create build artifacts in `build/`.

## Testing

```bash
make test
```

## IDE Setup

For proper IDE integration (clangd LSP support), the compilation database (`compile_commands.json`) is automatically generated when you run `make build` for the first time.

After the first build, reload your IDE/editor window for the changes to take effect.

