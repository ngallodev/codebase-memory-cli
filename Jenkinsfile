pipeline {
    agent any

    parameters {
        string(name: 'CBM_RELEASE_VERSION', defaultValue: '',
               description: 'Optional version passed to the release build, for example v0.11.0-rc.2')
        string(name: 'CBM_TEST_SUITES', defaultValue: '',
               description: '''Focused incremental test suites; leave blank for the full gate.
Reference list (space-separated):
arena hash_table dyn_array str_intern log str_util index_policy workspace platform diagnostics complexity subprocess private_file_lock lock_registry dump_verify ac extraction extraction_inheritance extraction_imports parse_coverage grammar_regression grammar_labels grammar_imports store_nodes store_edges store_search store_bulk store_pragmas store_checkpoint dump_verify_io cypher index_supervisor daemon project_lock version_cohort daemon_version daemon_runtime daemon_application daemon_bootstrap daemon_ipc language userconfig gitignore git_context discover graph_buffer registry pipeline importance index_format pipeline_semantic_manifest_repro call_reference_contract call_reference_language_complex_contract repro_call_scope_usages repro_call_argument_usages repro_reference_precision repro_call_argument_matrix_a repro_call_argument_matrix_b repro_call_node_behaviors repro_language_registry repro_call_node_manifest repro_lsp_ordered_signatures repro_lsp_ordered_local repro_ts_overload_return_chains repro_harness_cleanup repro_runner_filter cross_repo index_resilience fqn route_canon path_alias watcher lz4 zstd sqlite_writer artifact scope type_rep go_lsp c_lsp php_lsp cs_lsp cs_lsp_bench perl_lsp py_lsp kotlin_lsp rust_lsp py_lsp_bench py_lsp_stress py_lsp_scale ts_lsp java_lsp java_lsp_coverage store_arch traces configlink infrascan cli agent_clients legacy_agent_profiles config_json_like config_toml_edit config_yaml_edit config_text_edit activation_transaction system_info worker_pool parallel repro_lexical_binding_precision slab_alloc mem mem_events ui httpd security yaml semantic ast_profile simhash stack_overflow_a stack_overflow_b stack_overflow_c integration lang_contract edge_imports edge_structural lsp_resolution_probe node_creation_probe edge_types_probe convergence_probe matrix_known_classes matrix_new_constructs grammar_probe_a grammar_probe_b grammar_probe_c grammar_probe_d grammar_probe_e grammar_probe_f grammar_probe_g incremental''')
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
                    rm -f .jenkins-installed-cli
                    python3 tests/test_jenkins_daemon_recovery.py
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
        stage('Stop active CBM daemon') {
            steps {
                sh '''
                    set -eu
                    installed_cli="$HOME/.local/bin/codebase-memory-cli"
                    if [ -x "$installed_cli" ]; then
                        # Record recovery before stopping, even if Build or Install fails.
                        printf '%s\n' "$installed_cli" > .jenkins-installed-cli
                        "$installed_cli" daemon stop >/dev/null 2>&1 || true
                    fi
                '''
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
        stage('Install built CLI') {
            steps {
                sh '''
                    set -eu
                    install_dir="$HOME/.local/bin"
                    mkdir -p "$install_dir"
                    install -m 0755 build/c/codebase-memory-cli "$install_dir/codebase-memory-cli"
                    printf '%s\n' "$install_dir/codebase-memory-cli" > .jenkins-installed-cli
                '''
            }
        }
        stage('Prepare test runner') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh '''
                    set -eu
                    rm -rf build/c/test-logs
                    CBM_TEST_PHASE=prepare CBM_SKIP_PERF=1 scripts/test.sh
                '''
            }
        }
        stage('Test shards') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
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
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh 'scripts/ci/verify-shard-union.sh build/c/test-logs'
            }
        }
        stage('Post-test gates') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh 'CBM_TEST_PHASE=post CBM_TEST_SEAM_BINARY="$WORKSPACE/build/c-seams/codebase-memory-cli" scripts/test.sh'
            }
        }
        stage('Prepare focused test runner') {
            when {
                expression { params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh '''
                    set -eu
                    make -j"$(nproc)" -f Makefile.cbm build/c/test-runner
                '''
            }
        }
        stage('Focused test shards') {
            when {
                expression { params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                script {
                    def suites = params.CBM_TEST_SUITES.trim().split(/[\s,]+/).findAll { it }
                    if (!suites || !suites.every { it ==~ /[a-z0-9_]+/ }) {
                        error('CBM_TEST_SUITES must contain only suite names')
                    }

                    // Daemon-family suites share runtime/endpoint assumptions;
                    // keep them in one quiet shard and distribute the rest.
                    def daemonNames = ['cli', 'daemon', 'daemon_ipc', 'daemon_runtime',
                                       'daemon_application', 'daemon_bootstrap',
                                       'index_supervisor', 'watcher']
                    def daemon = suites.findAll { it in daemonNames }
                    def other = suites.findAll { !(it in daemonNames) }
                    def groups = [daemon, other].findAll { !it.isEmpty() }
                    def branches = [:]
                    groups.eachWithIndex { group, index ->
                        def shardSuites = group.join(' ')
                        branches["Focused shard ${index + 1}/${groups.size()}"] = {
                            withEnv(["CBM_FOCUSED_SUITE_ARGS=${shardSuites}"]) {
                                sh 'build/c/test-runner $CBM_FOCUSED_SUITE_ARGS'
                            }
                        }
                    }
                    parallel branches
                }
            }
        }
        stage('Package wrappers') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
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
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh 'scripts/test.sh --tsan'
            }
        }
        stage('Archive Jenkins qualification evidence') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
            steps {
                sh 'scripts/ci/publish-jenkins-evidence.sh'
                archiveArtifacts artifacts: 'jenkins-evidence/**', fingerprint: true
            }
        }
        stage('Archive Linux CLI artifact') {
            when {
                expression { !params.CBM_TEST_SUITES?.trim() }
            }
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
    post {
        always {
            sh '''
                set -eu
                if [ -s .jenkins-installed-cli ]; then
                    installed_cli=$(cat .jenkins-installed-cli)
                    "$installed_cli" daemon start >/dev/null 2>&1
                fi
            '''
        }
    }
}
