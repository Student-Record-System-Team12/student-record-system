#!/usr/bin/env bash
# One-off: protect main. Needs GitHub CLI (gh auth login) and admin rights on the repo.
# Usage: ./setup-branch-protection.sh divyaguptamo/student-record-system
set -euo pipefail
REPO="${1:?usage: $0 owner/repo}"
gh api -X PUT "repos/$REPO/branches/main/protection" --input - <<JSON
{
  "required_status_checks": {
    "strict": true,
    "contexts": ["build-and-test (ubuntu-latest)", "build-and-test (windows-latest)"]
  },
  "enforce_admins": false,
  "required_pull_request_reviews": {
    "required_approving_review_count": 1,
    "dismiss_stale_reviews": true
  },
  "restrictions": null,
  "required_conversation_resolution": true,
  "allow_force_pushes": false,
  "allow_deletions": false
}
JSON
echo "Branch protection applied to main on $REPO"
