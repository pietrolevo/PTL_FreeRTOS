# Contributing

## Contribution Process

### 1. Create an Issue

Before starting any work, **always create an issue** that describes:
- What you intend to do (feature, bugfix, documentation, etc.)
- The problem you're solving or the functionality you're adding
- Any relevant technical details

This allows for discussion of the approach before investing time in development.

### 2. Create a Branch

Once the issue has been discussed and approved, open a branch related to that issue

### 3. Write Commits

All commits must follow the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) specification.

**Format:**
```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

**Commit types:**
- `feat:` - new feature
- `fix:` - bug fix
- `docs:` - documentation changes
- `style:` - formatting, missing semicolons, etc.
- `refactor:` - code refactoring
- `test:` - adding or fixing tests
- `chore:` - changes to build process or auxiliary tools

**Examples:**
```bash
git commit -m "feat: add user authentication"
git commit -m "fix: correct calculation error in total"
git commit -m "docs: update README with installation instructions"
git commit -m "refactor(auth): improve session management"
```

### 3.1 License Header

Every new file added to the project **must include** the MIT License header at the top:

```
/*
 * Copyright 2025
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
```

### 4. Open a Merge Request

When your work is complete, you can open a Merge Request in two ways:

#### Option A: Via GitLab Web Interface

1. Push your branch to the remote repository:
   ```bash
   git push origin your-branch-name
   ```

2. GitLab will display a prompt with a link to create a Merge Request directly
3. Click the link or navigate to your project on GitLab
4. Click **"Create merge request"** button
5. Fill in the MR details (see below)

#### Option B: Using GitLab Push Options

You can create a Merge Request directly when pushing:

```bash
git push origin your-branch-name \
  -o merge_request.create \
  -o merge_request.title="Your MR Title" \
  -o merge_request.description="Closes #123" \
  -o merge_request.target=main
```

**Your Merge Request should include:**
- Reference to the related issue (e.g., "Closes #123")
- Clear description of changes
- Screenshots or examples if applicable
- Notes for reviewers

### 5. Code Review

Your MR must be **reviewed** before merging:

- Respond to reviewer comments
- Make requested changes
- Ensure all tests pass
- Keep the conversation constructive and professional

### 6. Merge

Once the review is approved:
- A maintainer will proceed with merging the branch
- The feature branch will be deleted after merge

## Checklist Before Opening an MR

- [ ] I created an issue and linked it to the MR
- [ ] Commits follow Conventional Commits
- [ ] All new files include the MIT License header
- [ ] Code is tested
- [ ] Documentation is updated
- [ ] Code follows the project's style guide
- [ ] All tests pass

