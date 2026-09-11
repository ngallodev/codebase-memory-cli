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
                withCredentials([string(credentialsId: 'github-token', variable: 'GH_TOKEN')]) {
                    sh '''
                        set -eu
                        export PATH="$WORKSPACE/.ci-venv/bin:$PATH"
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
        stage('Test') {
            steps {
                sh 'if [ -n "${CBM_TEST_SUITES:-}" ]; then scripts/test.sh --suites "$CBM_TEST_SUITES"; else scripts/test.sh; fi'
            }
        }
        stage('Package wrappers') {
            steps {
                sh 'scripts/ci/test-package-wrappers.sh'
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
