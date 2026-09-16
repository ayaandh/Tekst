# Contributing to Tekst

Thank you for your interest in contributing to **Tekst**.

Tekst is a programming language written in C++. Contributions are welcome, whether you want to fix a bug, improve the compiler, add a language feature, improve documentation, or help test the project.

## Before You Start

Before making a contribution:

1. Check the existing issues and merge requests.
2. Make sure your idea is not already being worked on.
3. For large language or compiler changes, open an issue first so the approach can be discussed.
4. Keep changes focused. Avoid combining unrelated changes in one merge request.

## Ways to Contribute

You can contribute by:

* Fixing bugs
* Adding or improving language features
* Improving the compiler
* Improving error messages
* Adding tests
* Improving documentation
* Improving build systems or developer tooling
* Reporting reproducible bugs
* Suggesting improvements to the language

You do not need to make code changes to contribute. Documentation, testing, and useful bug reports are valuable as well.

## Getting Started

### 1. Clone the Repository

```bash
git clone https://gitlab.com/ayaandh/Tekst.git
cd Tekst
```

### 2. Create a Branch

Create a branch for your change rather than working directly on the default branch.

```bash
git checkout -b feature/my-change
```

Use a descriptive branch name, for example:

```text
feature/functions
fix/parser-error
docs/getting-started
test/string-literals
```

### 3. Build Tekst

Follow the build instructions in the repository's `README.md`.

Make sure the project builds successfully before making your changes.

### 4. Make Your Changes

Make the smallest reasonable change that solves the problem.

For compiler or language changes, consider all relevant stages, such as:

```text
Source
  ↓
Lexer
  ↓
Parser
  ↓
AST / Representation
  ↓
Semantic Analysis
  ↓
Code Generation / Execution
```

If your change affects multiple stages, make sure each affected stage continues to work correctly.

## Coding Guidelines

Tekst is written in C++, so follow the existing style of the project.

In general:

* Write clear and readable C++.
* Prefer simple solutions over unnecessary complexity.
* Keep functions focused.
* Use meaningful names.
* Avoid unrelated refactoring.
* Do not introduce dependencies without a good reason.
* Follow the formatting and conventions already used in the surrounding code.
* Add comments when the reasoning behind code is not obvious, rather than commenting every line.

Consistency with the existing codebase is more important than introducing a personal coding style.

## Language Changes

Changes to Tekst's syntax or semantics should be treated carefully.

When proposing a new language feature, explain:

* What problem it solves
* What the syntax looks like
* What the expected behavior is
* Why the feature belongs in the language
* How it interacts with existing features
* Whether it introduces breaking changes

For example:

```tekst
let name = "Tekst"
print(name)
```

A language feature should ideally include tests demonstrating both valid and invalid usage.

## Testing

Every bug fix should include a test when practical.

New language features should include tests covering:

* Normal usage
* Edge cases
* Invalid syntax
* Invalid semantics
* Relevant error messages

Before submitting a merge request, make sure existing tests still pass.

If the project does not yet have tests covering the affected area, consider adding them as part of your contribution.

## Reporting Bugs

When reporting a bug, include enough information to reproduce it.

A useful bug report contains:

````text
### Description

What happened?

### Expected Behavior

What should have happened?

### Steps to Reproduce

1. ...
2. ...
3. ...

### Input

```tekst
your code here
````

### Output

```text
actual output
```

### Environment

* OS:
* Compiler:
* Tekst version/commit:

````

Avoid vague reports such as "the compiler doesn't work." A minimal reproducible example is much more useful.

## Feature Requests

For feature requests, explain the problem before proposing the solution.

A useful feature request should include:

- The problem
- The proposed feature
- Example Tekst code
- Expected behavior
- Possible alternatives

Feature requests do not automatically mean the feature will be accepted. Language design requires considering the long-term consistency of Tekst.

## Commits

Keep commits focused and descriptive.

Good:

```text
Add string literal parsing
Fix parser handling of empty arguments
Add tests for integer literals
Improve lexer error messages
````

Avoid commits such as:

```text
stuff
changes
update
fix
asdf
```

Try to make each commit represent one logical change.

## Merge Requests

When your change is ready:

1. Push your branch.
2. Open a merge request against the default branch.
3. Explain what changed and why.
4. Mention related issues if applicable.
5. Include tests or explain why tests are not applicable.
6. Be prepared to make changes based on review.

A useful merge request description can follow this structure:

```markdown
## What changed?

Describe the change.

## Why?

Explain the problem this solves.

## Testing

Explain how the change was tested.

## Related Issues

Closes #123
```

## Review

Code review is part of contributing to Tekst.

Reviewers may request:

* Changes to the implementation
* Additional tests
* Documentation
* A different approach
* Clarification of language behavior

Please keep discussions focused on the code and design.

## Documentation

Documentation contributions are welcome.

When documenting a language feature, include examples where useful and keep examples consistent with the actual behavior of the compiler.

If documentation and implementation disagree, the implementation should not be assumed to be correct. Report the discrepancy so it can be resolved.

## Breaking Changes

Changes that modify existing language behavior or syntax should be clearly identified.

Examples include:

* Removing syntax
* Changing the meaning of existing syntax
* Changing compiler behavior
* Changing standard-library behavior
* Making previously valid programs invalid

Discuss significant breaking changes before implementing them.

## Security

If you discover a security vulnerability, do not publicly disclose sensitive details in an issue.

Contact the project maintainer privately so the issue can be investigated responsibly.

## Code of Conduct

Be respectful to other contributors.

Constructive disagreement is welcome. Personal attacks, harassment, discrimination, and deliberately disruptive behavior are not.

## Final Checklist

Before opening a merge request, check:

* [ ] The project builds successfully.
* [ ] My change is focused and does not contain unrelated modifications.
* [ ] I tested the change.
* [ ] I added tests where appropriate.
* [ ] Documentation has been updated if necessary.
* [ ] Existing functionality still works.
* [ ] My commit messages describe the changes clearly.
* [ ] The merge request explains what changed and why.

Thank you for contributing to **Tekst**.
