# Student Record System

![CI](https://github.com/divyaguptamo/student-record-system/actions/workflows/ci.yml/badge.svg)

A console application in C for managing student records, grades and attendance
(Software Engineering mini project, Team 12).

## Features

- Login with role-based menus (Admin / Student), SHA-256 hashed passwords, masked password entry
- Account lockout after 3 failed login attempts
- Student records: add and view from the menu, with duplicate roll number check (update and delete are in the service layer)
- Attendance: mark, update, view per student, percentage per subject
- Input validation for roll numbers, dates, marks, phone numbers and free text
- Append-only audit log of security and data-changing events

## Team and module ownership

| Member | Module |
|---|---|
| Mem 1 | Authentication and access control, CI/CD, repo setup |
| Mem 2 | Student records, storage layer, audit log |
| Mem 3 | Grades, search and reports |
| Mem 4 | Attendance, input validation, console menu |

## Build and run

Requires GCC and make (Linux, macOS, or Windows with MinGW and make).

```
make all     # build the application (outputs ./app)
make test    # build and run all unit and integration tests
make clean   # remove build output
./app        # run
```

On first run, if no users exist, a default admin account is created:
username `admin`, password `admin123`. **Change this before any real use.**

## Usage

| Role | Menu options |
|---|---|
| Admin | Mark / update / view attendance, add / view student record, create student login |
| Student | View own record, view own attendance |

Data files are created in the working directory: `users.dat`, `students.dat`,
`attendance.csv`, `audit.log`. They are git-ignored.

## Project layout

```
src/            application source (main.c is the console menu)
include/        header files
tests/          unit and integration tests (Unity)
tests/manual/   manual test helpers (password masking)
third_party/    Unity test framework, SHA-256
docs/           Test Plan and manual test evidence
.github/        CI workflow, PR template
```

## Testing

`make test` builds each `tests/test_*.c` into `build/` and runs it. Test names
carry the Test Plan case ID (e.g. `TC_Sec_05a`), so failures trace back to
`docs/Student_Record_System_Test_Plan.docx`. CI also runs cppcheck and gcov
coverage on Linux (reported, not blocking).

## Workflow

- `main` is protected: no direct pushes.
- Branch from `main` as `feature/<module>-<name>`.
- Commit messages start with the requirement ID, e.g. `STU-F003: add lockout counter`.
- Open a pull request into `main` using the PR template.
- A PR needs a green CI run (Linux and Windows) and one approving review from
  a teammate other than the author before it can be merged.
- Fix a failing pipeline before starting new work.
