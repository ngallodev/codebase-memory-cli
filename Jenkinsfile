pipeline {
    agent any

    parameters {
        string(name: 'CBM_RELEASE_VERSION', defaultValue: '',
               description: 'Optional version passed to the release build, for example v0.11.0-rc.2')
    }

    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        disableConcurrentBuilds()
        timeout(time: 240, unit: 'MINUTES')
    }

    stages {
        stage('Lint') {
            steps {
                sh '''
                    set -eu
                    scripts/lint.sh --ci CLANG_FORMAT=clang-format-20
                '''
            }
        }
        stage('Memory lint') {
            steps {
                sh 'scripts/ci/lint-mem.sh clang-tidy-22'
            }
        }
        stage('Security static') {
            steps {
                sh '''
                    set -eu
                    scripts/security-audit.sh
                    scripts/security-ui.sh
                    scripts/security-vendored.sh
                '''
            }
        }
        stage('Install Python CI tools') {
            steps {
                sh 'CBM_CI_VENV="$WORKSPACE/.ci-venv" scripts/ci/install-python-tools.sh'
            }
        }
        stage('License gate') {
            steps {
                withCredentials([usernamePassword(credentialsId: 'github-https-token',
                                                   usernameVariable: 'GITHUB_USER',
                                                   passwordVariable: 'GH_TOKEN')]) {
                    sh '''
                        set -eu
                        export PATH="$WORKSPACE/.ci-venv/bin:$PATH"
                        gh api user --jq .login >/dev/null
                        gh api rate_limit --jq '"GitHub API rate limit: " + (.resources.core.remaining|tostring) + "/" + (.resources.core.limit|tostring) + " remaining"'
                        scripts/license-gate.sh --selftest
                        scripts/license-gate.sh
                        scripts/audit-license-provenance.py
                    '''
                }
            }
        }
        stage('Build') {
            steps {
                sh '''
                    set -eu
                    if [ -n "${CBM_RELEASE_VERSION:-}" ]; then
                        scripts/build.sh --version "$CBM_RELEASE_VERSION"
                    else
                        scripts/build.sh
                    fi
                '''
            }
        }
        stage('Prepare test runner') {
            steps {
                sh '''
                    set -eu
                    rm -rf build/c/test-logs
                    CBM_TEST_PHASE=prepare CBM_SKIP_PERF=1 scripts/test.sh
                '''
            }
        }
        stage('Test shards') {
            // Three shards share this 56-core host; nine workers each keeps
            // the aggregate test fan-out at 27, below the requested 28-core cap.
            parallel {
                stage('Build seam test binary') {
                    steps {
                        sh '''
                            set -eu
                            # The three test shards use 27 workers; keep the
                            # concurrent seam compile within the 28-core cap.
                            make -j1 -f Makefile.cbm cbm \
                                TEST_SEAMS=1 BUILD_DIR=build/c-seams
                        '''
                    }
                }
                stage('Test shard 1/3') {
                    steps {
                        sh 'CBM_TEST_PHASE=shard CBM_TEST_SHARD=1/3 CBM_TEST_LEG=jenkins-main CBM_TEST_LOG_DIR=build/c/test-logs/shard-1 CBM_TEST_PAR_JOBS=9 CBM_SKIP_PERF=1 scripts/test.sh'
                    }
                }
                stage('Test shard 2/3') {
                    steps {
                        sh 'CBM_TEST_PHASE=shard CBM_TEST_SHARD=2/3 CBM_TEST_LEG=jenkins-main CBM_TEST_LOG_DIR=build/c/test-logs/shard-2 CBM_TEST_PAR_JOBS=9 CBM_SKIP_PERF=1 scripts/test.sh'
                    }
                }
                stage('Test shard 3/3') {
                    steps {
                        sh 'CBM_TEST_PHASE=shard CBM_TEST_SHARD=3/3 CBM_TEST_LEG=jenkins-main CBM_TEST_LOG_DIR=build/c/test-logs/shard-3 CBM_TEST_PAR_JOBS=9 CBM_SKIP_PERF=1 scripts/test.sh'
                    }
                }
            }
        }
        stage('Verify test shards') {
            steps {
                sh 'scripts/ci/verify-shard-union.sh build/c/test-logs'
            }
        }
        stage('Post-test gates') {
            steps {
                sh 'CBM_TEST_PHASE=post CBM_TEST_SEAM_BINARY="$WORKSPACE/build/c-seams/codebase-memory-cli" scripts/test.sh'
            }
        }
        stage('Package wrappers') {
            steps {
                sh '''
                    set -eu
                    # The local Jenkins service does not inherit interactive
                    # shell profiles. Keep the toolchain location explicit;
                    # the wrapper script verifies the exact go.mod version.
                    export PATH="/usr/local/go/bin:$PATH"
                    scripts/ci/test-package-wrappers.sh
                '''
            }
        }
        stage('Thread sanitizer') {
            steps {
                sh 'scripts/test.sh --tsan'
            }
        }
        stage('Archive Linux CLI artifact') {
            steps {
                sh '''
                    set -eu
                    artifact_dir='jenkins-artifacts/codebase-memory-cli-linux-amd64'
                    rm -rf "$artifact_dir"
                    mkdir -p "$artifact_dir"
                    cp build/c/codebase-memory-cli "$artifact_dir/codebase-memory-cli"
                    chmod 0755 "$artifact_dir/codebase-memory-cli"
                    sha256sum "$artifact_dir/codebase-memory-cli" > "$artifact_dir/SHA256SUMS"
                    git rev-parse HEAD > "$artifact_dir/source-revision"
                    "$artifact_dir/codebase-memory-cli" --version > "$artifact_dir/version.txt"
                '''
                archiveArtifacts artifacts: 'jenkins-artifacts/codebase-memory-cli-linux-amd64/**', fingerprint: true
            }
        }
    }
}
