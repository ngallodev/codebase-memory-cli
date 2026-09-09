pipeline {
    agent any

    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        disableConcurrentBuilds()
        timeout(time: 45, unit: 'MINUTES')
    }

    stages {
        stage('Build') {
            steps { sh 'scripts/build.sh' }
        }
        stage('Test') {
            steps { sh 'scripts/test.sh' }
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
