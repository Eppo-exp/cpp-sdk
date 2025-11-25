# Contributing to Eppo C++ SDK

Thank you for your interest in contributing to the Eppo C++ SDK! This document provides guidelines for code formatting and development practices.

## Code Formatting

We use automated code formatting tools to ensure consistent code style across the project. This makes code reviews easier and helps maintain code quality.

### Quick Start

Before committing code, run:

```bash
make format
```

### Tools

We use two complementary tools for code formatting:

1. **clang-format** - Handles C++ code formatting (indentation, spacing, line breaks, etc.)
2. **EditorConfig** - Handles basic editor settings (line endings, trailing whitespace, charset)

**Note on clang-format versions:** The CI uses clang-format 18 for consistency. Most modern versions of clang-format (14+) should produce similar results, but if you encounter CI formatting failures, ensure you're using a compatible version.

### Setting Up Your Development Environment

#### 1. Install clang-format

**macOS:**
```bash
brew install clang-format
```

**Ubuntu/Debian:**
```bash
sudo apt-get install clang-format
```

**Fedora/RHEL:**
```bash
sudo dnf install clang-tools-extra
```

#### 2. Configure Your Editor (Optional but Recommended)

Most modern editors support automatic formatting on save:

**VSCode:**
- Install the "C/C++" extension
- Add to your `.vscode/settings.json`:
```json
{
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_path": "/usr/bin/clang-format",
  "C_Cpp.clang_format_style": "file"
}
```

**CLion:**
- Settings → Editor → Code Style
- Enable "Enable ClangFormat"
- Check "Format on save"

**Vim/Neovim:**
```vim
" Add to your .vimrc
autocmd FileType cpp setlocal formatprg=clang-format
```

**Emacs:**
```elisp
;; Add to your .emacs
(require 'clang-format)
(global-set-key (kbd "C-c f") 'clang-format-region)
```

#### 3. Install Pre-commit Hook (Optional)

To automatically format code before each commit:

```bash
ln -s ../../scripts/pre-commit-format .git/hooks/pre-commit
chmod +x scripts/pre-commit-format
```

This hook will:
- Automatically format staged C++ files with clang-format
- Add the formatted files back to the commit
- Allow you to skip formatting with `git commit --no-verify` if needed

## Makefile Targets

### Format all source files

```bash
make format
```

This command formats all C++ source files in `src/`, `test/`, and `examples/` directories using clang-format.

### Check formatting (without modifying files)

```bash
make format-check
```

This command checks if all files are properly formatted. It exits with an error if any files need formatting. This is useful for:
- Pre-commit checks
- CI/CD pipelines
- Verifying formatting before creating a PR

## Code Style Guidelines

Our `.clang-format` configuration is based on Google style with these customizations:

- **Indentation:** 4 spaces (no tabs)
- **Line length:** 120 characters maximum
- **Braces:** Opening braces on the same line (Attach style)
- **Pointers/References:** Left-aligned (`int* ptr` not `int *ptr`)
- **Namespace indentation:** None
- **Standard:** C++17

### EditorConfig Settings

The `.editorconfig` file ensures:
- Unix-style line endings (LF)
- UTF-8 encoding
- Final newline at end of files
- Trailing whitespace is trimmed
- Consistent indentation across file types

## Continuous Integration

All pull requests are automatically checked for proper formatting using GitHub Actions. The CI pipeline will fail if code is not properly formatted.

To ensure your PR passes the formatting check, run `make format-check` before pushing:

```bash
make format-check
# If this passes, your PR will pass the CI formatting check
```

## Workflow for Contributors

1. **Write your code**
2. **Format before committing:**
   ```bash
   make format
   ```
3. **Verify formatting:**
   ```bash
   make format-check
   ```
4. **Run tests:**
   ```bash
   make test
   ```
5. **Commit and push:**
   ```bash
   git add .
   git commit -m "Your commit message"
   git push
   ```

## Troubleshooting

### "clang-format: command not found"

Install clang-format using the instructions in the "Install clang-format" section above.

### Formatting check fails in CI but passes locally

Make sure you're using a compatible version of clang-format. The CI uses the version available in Ubuntu's apt repositories. You can check your version with:

```bash
clang-format --version
```

### I want to commit without formatting

If you need to commit without running the pre-commit hook (not recommended):

```bash
git commit --no-verify
```

Note: The CI will still check formatting, so your PR may fail if code is not properly formatted.

### Editor is not respecting .editorconfig

Make sure your editor has EditorConfig support:
- VSCode: Install the "EditorConfig for VS Code" extension
- CLion: Built-in support (enabled by default)
- Vim: Install the editorconfig-vim plugin
- Emacs: Install editorconfig-emacs

## Questions?

If you have questions about code formatting or contributing, please:
- Open an issue on GitHub
- Check existing issues for similar questions
- Refer to the [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)

Thank you for helping keep the codebase clean and consistent!
