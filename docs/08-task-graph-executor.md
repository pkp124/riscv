# Design Proposal 08: Bare-Metal Task Graph Executor in C

## 1. Executive Summary

This proposal defines a small, deterministic, Taskflow-inspired executor for
bare-metal RISC-V firmware written in C. The goal is not to clone desktop
Taskflow, but to capture the useful idea: application work is expressed as a
task graph with explicit dependencies, then executed by a policy-driven
scheduler on single-hart, SMP, and future AMP/many-core platforms.

The design keeps the existing project constraints:

- C first, no C++ runtime requirement
- no heap allocation, no standard library dependency, no OS primitives
- test-driven development using CMake + CTest and simulator output matching
- cross-platform validation on the existing RISC-V testbench
- a path toward AMP systems with heterogeneous compute domains
- functional-safety-ready engineering practices, without claiming
  certification

**Approval status:** Proposed. This document, the roadmap phase, and the task
manifest require project-owner approval before implementation begins.

---

## 2. Requirements Review and Opinion

### 2.1 Stated Requirements

1. Build a simple Taskflow-like project for bare-metal systems.
2. Implement it in C.
3. Produce a project plan, design, milestones, and tasks.
4. Use a task-management system that works well for agents.
5. Require owner approval for designs and milestones.
6. Use test-driven development.
7. Use this repository's RISC-V simulator testbench for validation.
8. Keep many-core AMP systems in mind.
9. Support different scheduling policies.
10. Make the design functional-safety ready where possible.

### 2.2 Engineering Opinion

This is a good direction if the executor remains small and deterministic. The
bare-metal version should favor static configuration, bounded queues, explicit
error states, and scheduler policies that can be tested exhaustively at small
sizes. It should not become an RTOS, a general-purpose thread library, or a C++
framework port.

The most useful first deliverable is a single-hart static DAG executor. That
gives us the API, graph validation, failure semantics, and tests before any
multi-hart scheduling complexity is added. SMP and AMP should build on the same
graph model rather than introducing a second runtime.

### 2.3 Non-Goals

- Dynamic task creation using `malloc()` or heap-backed containers
- Preemptive scheduling
- Blocking I/O primitives
- POSIX threads, condition variables, or OS dependencies
- General-purpose dataflow buffers in the first implementation
- Certification claims such as ISO 26262 ASIL compliance
- Runtime parsing of graph files on bare metal

---

## 3. Approval and Change-Control Model

The project uses approval gates so agents can prepare work without silently
changing the product direction.

| Artifact | Purpose | Approval Rule |
|----------|---------|---------------|
| `docs/08-task-graph-executor.md` | Human-readable design source of truth | Must be approved before implementation |
| `ROADMAP.md` Phase 11 | Milestones and exit criteria | Must be approved before implementation |
| `tasks/task-graph-executor.yaml` | Agent-friendly task manifest | Tasks must be `approved` before coding |

### 3.1 Allowed Status Values

- `proposed`: documented but not approved
- `approved`: ready for implementation
- `in_progress`: an agent is actively working on it
- `blocked`: cannot proceed until a dependency or decision is resolved
- `done`: completed and verified
- `rejected`: explicitly declined

### 3.2 Agent Workflow

Before implementing a task graph executor change, an agent must:

1. Read this design document and `tasks/task-graph-executor.yaml`.
2. Confirm the task and its parent milestone are `approved`.
3. Add or update CTest coverage first.
4. Run the new test and capture the expected red result when feasible.
5. Implement the smallest change needed to pass the test.
6. Run the relevant simulator test preset.
7. Update task evidence in the PR summary and, if appropriate, the task
   manifest.

No implementation should begin from a `proposed` milestone unless the user
explicitly approves it in the conversation or by changing the status in the
tracked artifacts.

---

## 4. Top-Level Requirements

### 4.1 Functional Requirements

| ID | Requirement |
|----|-------------|
| TGE-F-001 | Provide a C API for defining static task graphs. |
| TGE-F-002 | Support directed acyclic graphs with explicit dependency edges. |
| TGE-F-003 | Execute ready tasks on a single hart with deterministic behavior. |
| TGE-F-004 | Detect invalid graph configuration before execution starts. |
| TGE-F-005 | Support bounded node, edge, queue, and domain capacities. |
| TGE-F-006 | Report task completion, task failure, and graph failure states. |
| TGE-F-007 | Provide pluggable scheduling policies. |
| TGE-F-008 | Support scheduler metadata such as priority, affinity, deadline class, and capability requirements. |
| TGE-F-009 | Support SMP execution using existing hart, atomic, spinlock, and barrier primitives. |
| TGE-F-010 | Support AMP execution domains where different clusters may run different firmware and scheduler policies. |
| TGE-F-011 | Support heterogeneous capability routing, including RVV-capable domains. |
| TGE-F-012 | Emit structured test output suitable for CTest regex validation. |
| TGE-F-013 | Provide deterministic tracing hooks that can be compiled out. |
| TGE-F-014 | Provide a safe shutdown or safe-state hook when graph execution fails. |

### 4.2 Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| TGE-NF-001 | No heap allocation. |
| TGE-NF-002 | No dependency on `<stdio.h>`, `<stdlib.h>`, `<string.h>`, or OS services. |
| TGE-NF-003 | Fixed-width integer types for public data structures. |
| TGE-NF-004 | Bounded execution metadata with compile-time or initialization-time capacity checks. |
| TGE-NF-005 | MISRA-C-friendly style where practical. |
| TGE-NF-006 | Explicit ownership of shared data between tasks. |
| TGE-NF-007 | Stable requirement-to-test traceability. |
| TGE-NF-008 | Simulator tests must run through the existing CMake + CTest testbench. |
| TGE-NF-009 | Cross-platform behavior should be validated first on QEMU and Spike, then AMP-capable platforms. |

### 4.3 Functional-Safety-Readiness Requirements

| ID | Requirement |
|----|-------------|
| TGE-SAFE-001 | No unbounded memory allocation or unbounded queues. |
| TGE-SAFE-002 | Validate graph acyclicity and capacity before execution. |
| TGE-SAFE-003 | Preserve a traceable terminal state for every task. |
| TGE-SAFE-004 | Provide hooks for watchdog servicing and watchdog failure response. |
| TGE-SAFE-005 | Provide optional runtime assertions for internal invariants. |
| TGE-SAFE-006 | Support fault-injection tests for task failure, stuck task detection, and invalid graph handling. |
| TGE-SAFE-007 | Document assumptions, failure modes, and safe-state behavior. |
| TGE-SAFE-008 | Keep scheduler policies small enough to review and test independently. |
| TGE-SAFE-009 | Preserve freedom-from-interference boundaries for AMP domains where supported by memory maps or PMP. |

---

## 5. Architecture

### 5.1 Conceptual Model

```
              static task graph
        +-----------------------------+
        | nodes, edges, metadata      |
        | capacities, domains         |
        +--------------+--------------+
                       |
                       v
        +-----------------------------+
        | graph validator             |
        | - capacity checks           |
        | - acyclicity check          |
        | - dependency counters       |
        +--------------+--------------+
                       |
                       v
        +-----------------------------+
        | executor                    |
        | - ready queues              |
        | - task states               |
        | - completion propagation    |
        +--------------+--------------+
                       |
                       v
        +-----------------------------+
        | scheduling policy           |
        | FIFO, priority, affinity,   |
        | domain, safety-static table |
        +--------------+--------------+
                       |
                       v
        +-----------------------------+
        | platform back end           |
        | single hart, SMP, AMP       |
        +-----------------------------+
```

### 5.2 Core Data Structures

The executor should be configured with caller-owned storage. This avoids heap
allocation and makes memory sizing visible to tests and safety analysis.

```c
typedef enum {
    TGE_STATUS_OK = 0,
    TGE_STATUS_INVALID_GRAPH,
    TGE_STATUS_CAPACITY_EXCEEDED,
    TGE_STATUS_TASK_FAILED,
    TGE_STATUS_DEADLOCK,
    TGE_STATUS_TIMEOUT
} tge_status_t;

typedef enum {
    TGE_TASK_CREATED = 0,
    TGE_TASK_READY,
    TGE_TASK_RUNNING,
    TGE_TASK_DONE,
    TGE_TASK_FAILED,
    TGE_TASK_SKIPPED
} tge_task_state_t;

typedef struct {
    uint16_t task_id;
    uint16_t domain_id;
    uint8_t priority;
    uint32_t capability_mask;
    void *user_data;
} tge_task_context_t;

typedef tge_status_t (*tge_task_fn_t)(tge_task_context_t *ctx);
```

The exact public header should be finalized during implementation, but the API
should keep the following properties:

- every task has a stable numeric ID
- graph capacity is visible at initialization
- dependency edges are explicit
- task state is queryable after execution
- policy selection is an initialization parameter
- no task registration path allocates memory

### 5.3 Graph Representation

The first implementation should use fixed arrays:

- task descriptor array
- edge descriptor array
- incoming dependency count array
- outgoing adjacency index array or compact edge scan
- ready queue storage supplied by the caller

For small graphs, a compact edge scan is acceptable and easier to validate. For
larger many-core graphs, an adjacency representation can be added once tests
demonstrate the need.

### 5.4 Scheduler Policy Interface

Scheduling policy should be isolated from graph correctness. The executor owns
state transitions and dependency propagation; the policy chooses which ready
task to run next.

Initial policies:

| Policy | Purpose | Determinism |
|--------|---------|-------------|
| FIFO | Simple baseline, easiest to test | deterministic |
| Priority | Higher-priority ready tasks run first | deterministic with tie-break by task ID |
| Affinity | Prefer tasks mapped to a hart or domain | deterministic per domain |
| Static safety table | Execute only according to a precomputed table | deterministic |

Future policies:

- round-robin per domain
- earliest-deadline-class first
- work stealing for many-core SMP
- energy-aware or memory-locality-aware scheduling

### 5.5 Single-Hart Execution

Single-hart execution is the reference semantic model:

1. Validate graph.
2. Initialize dependency counters and task states.
3. Enqueue all zero-dependency tasks.
4. Select a ready task using the active policy.
5. Run the task to completion.
6. Propagate completion to dependent tasks.
7. Repeat until every task is terminal or the graph fails.

All SMP and AMP behavior must be explainable as a parallelization of this
reference model.

### 5.6 SMP Execution

SMP execution can use existing project primitives:

- hart boot and rendezvous from Phase 4
- AMO/LR-SC atomics
- spinlocks and barriers
- structured console output

The first SMP scheduler should be conservative:

- one shared ready queue protected by a spinlock
- atomic dependency counters
- one active task per hart
- deterministic completion checks in tests

Later, many-core scaling can add per-hart queues and optional work stealing.

### 5.7 AMP and Many-Core Execution

AMP support introduces execution domains. A domain may represent a cluster,
firmware image, memory map, ISA profile, or safety partition.

```
Task graph
  |
  +-- domain 0: rv64gc control cluster
  |      policy: priority
  |      memory: private SRAM + shared mailbox
  |
  +-- domain 1: rv64gcv accelerator cluster
  |      policy: affinity/RVV
  |      memory: private scratchpad + shared SRAM
  |
  +-- domain 2: safety monitor cluster
         policy: static safety table
         memory: protected monitor region
```

AMP edges can be local or remote:

- local edge: producer and consumer are in the same domain
- remote edge: producer completion is signaled through shared memory, mailbox,
  or interrupt

The graph model should support AMP metadata early, but the first implementation
does not need to run on an AMP platform. The planned SystemC AMP platform from
Phase 8 is the natural long-term backend for cross-domain executor tests.

### 5.8 Error Handling

The executor should fail explicitly. A task returning non-OK status marks that
task failed. Dependent tasks should be skipped unless a future policy explicitly
supports recovery nodes.

Recommended first semantics:

- any task failure transitions graph execution to failed
- ready but not yet run tasks remain queryable
- dependents of failed tasks become skipped
- the executor calls an optional safe-state hook
- CTest validates the emitted failure summary

---

## 6. Functional-Safety Readiness

This design can be made safety-ready, but it should not claim certification.
Certification depends on process, tool qualification, requirements traceability,
coverage evidence, safety analysis, and independent review.

Recommended safety-oriented practices:

| Area | Planned Practice |
|------|------------------|
| Requirements | Stable requirement IDs in this document |
| Traceability | Map requirement IDs to task IDs and CTest names |
| Memory safety | static allocation and explicit capacities |
| Control flow | no recursion in executor core |
| Determinism | stable tie-break rules for every policy |
| Diagnostics | structured task and graph terminal states |
| Watchdog | hooks before/after task execution and on graph failure |
| Fault injection | tests for bad graph, task failure, queue full, timeout |
| Static analysis | cppcheck now; MISRA checks optional in advanced validation |
| Freedom from interference | domain metadata and future PMP/memory-map enforcement |

Safety-related design reviews should happen at the end of each implementation
milestone before broadening the scheduler.

---

## 7. Test Strategy Using the Existing RISC-V Testbench

All implementation work must follow Red-Green-Refactor:

1. Add a CTest entry with expected output.
2. Run it and capture the failing result when practical.
3. Implement the minimum firmware/runtime change.
4. Re-run the relevant preset.
5. Refactor while keeping tests green.

### 7.1 Initial Test Platforms

| Stage | Platforms |
|-------|-----------|
| Single-hart executor | QEMU single-core, Spike single-core |
| Scheduler policies | QEMU single-core, Spike single-core |
| SMP executor | QEMU SMP, Spike SMP |
| RVV affinity | QEMU RVV, Spike RVV where supported |
| AMP domains | SystemC AMP after Phase 8, then Renode/gem5 where suitable |

### 7.2 Test Output Format

Firmware should use structured output:

```text
[TGE] Policy: fifo
[TEST] Task graph single node: PASS
[TEST] Task graph diamond dependency: PASS
[TEST] Task graph invalid cycle: PASS
[RESULT] Phase 11 task graph: 3/3 PASS
```

CTest should match specific PASS lines and use explicit fail patterns:

```cmake
set_tests_properties(phase11_qemu_tge_diamond PROPERTIES
  PASS_REGULAR_EXPRESSION "\\[TEST\\] Task graph diamond dependency: PASS"
  FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
  TIMEOUT 30
  LABELS "phase11;qemu;tge;functional"
)
```

---

## 8. Proposed Implementation Layout

```
app/
├── include/
│   └── tge/
│       ├── tge.h                 # public executor API
│       ├── tge_config.h          # compile-time capacities/defaults
│       ├── tge_policy.h          # scheduler policy interface
│       └── tge_trace.h           # optional trace hooks
└── src/
    └── tge/
        ├── tge.c                 # graph validation and execution
        ├── tge_policy_fifo.c
        ├── tge_policy_priority.c
        ├── tge_policy_affinity.c
        └── tge_tests.c           # bare-metal test workload entry points

tests/
└── CMakeLists.txt                # Phase 11 CTest entries

tasks/
└── task-graph-executor.yaml      # agent-friendly task manifest
```

The implementation may start with a single `app/include/tge.h` and
`app/src/tge.c` if that is simpler. Split files only when tests show the policy
code or tracing code has grown enough to justify separation.

---

## 9. Milestones

All milestones are proposed and require approval before implementation.

### 11.0 Planning and Approval

**Goal:** Establish the design, roadmap, and task manifest.

**Tasks:**

1. Add this design document.
2. Add Phase 11 to `ROADMAP.md`.
3. Add `tasks/task-graph-executor.yaml`.
4. Wait for owner approval before implementation.

**Exit Criteria:**

- Design reviewed by the project owner
- Milestone statuses changed from `proposed` to `approved`
- Open top-level requirements resolved or accepted as deferred

### 11.1 Single-Hart Static DAG Executor

**Goal:** Execute a static DAG deterministically on one hart.

**Tests first:**

- `phase11_qemu_tge_single_node`
- `phase11_qemu_tge_linear_chain`
- `phase11_qemu_tge_diamond`
- `phase11_qemu_tge_capacity_reject`
- `phase11_spike_tge_single_node`
- `phase11_spike_tge_diamond`

**Implementation tasks:**

1. Define the minimal public C API.
2. Add caller-owned storage initialization.
3. Add graph validation and dependency counters.
4. Add FIFO ready queue.
5. Add terminal task state reporting.

### 11.2 Invalid Graph and Failure Semantics

**Goal:** Make failure modes explicit and testable.

**Tests first:**

- `phase11_qemu_tge_cycle_reject`
- `phase11_qemu_tge_missing_task_reject`
- `phase11_qemu_tge_task_failure`
- `phase11_qemu_tge_failure_skips_dependents`
- `phase11_spike_tge_cycle_reject`

**Implementation tasks:**

1. Detect cycles before execution.
2. Reject edges that reference invalid tasks.
3. Define failed/skipped dependent semantics.
4. Add optional safe-state callback.

### 11.3 Scheduler Policy Interface

**Goal:** Separate scheduler selection from graph correctness.

**Tests first:**

- `phase11_qemu_tge_fifo_policy`
- `phase11_qemu_tge_priority_policy`
- `phase11_qemu_tge_affinity_metadata`
- `phase11_spike_tge_priority_policy`

**Implementation tasks:**

1. Define scheduler policy interface.
2. Move FIFO scheduling behind the policy interface.
3. Add priority scheduling with deterministic tie-breaks.
4. Add affinity metadata without requiring SMP yet.

### 11.4 SMP Task Execution

**Goal:** Execute ready tasks across multiple harts.

**Tests first:**

- `phase11_qemu_tge_smp_parallel_ready`
- `phase11_qemu_tge_smp_dependency_release`
- `phase11_qemu_tge_smp_atomic_completion`
- `phase11_qemu_tge_smp_stress`
- `phase11_spike_tge_smp_parallel_ready`

**Implementation tasks:**

1. Reuse Phase 4 hart boot and barriers.
2. Protect the initial shared ready queue with existing spinlock primitives.
3. Use atomic dependency completion.
4. Validate final graph state on hart 0.

### 11.5 AMP Domain Model

**Goal:** Model task placement on heterogeneous domains and prepare for
cross-cluster execution.

**Tests first:**

- `phase11_qemu_tge_domain_metadata`
- `phase11_qemu_tge_rvv_capability_route`
- `phase11_systemc_tge_amp_mailbox`
- `phase11_systemc_tge_amp_remote_dependency`

**Implementation tasks:**

1. Add domain descriptors to graph configuration.
2. Add capability masks for task placement.
3. Route RVV-marked tasks to RVV-capable domains in metadata tests.
4. Use SystemC AMP mailbox tests when Phase 8 infrastructure is available.

### 11.6 Functional-Safety Readiness Layer

**Goal:** Add hooks and evidence patterns expected by safety-oriented firmware.

**Tests first:**

- `phase11_qemu_tge_watchdog_hook`
- `phase11_qemu_tge_trace_ring`
- `phase11_qemu_tge_fault_injection_task_failure`
- `phase11_qemu_tge_static_safety_policy`

**Implementation tasks:**

1. Add optional watchdog hook points.
2. Add bounded trace ring support.
3. Add fault-injection test helpers.
4. Add static safety-table scheduler policy.
5. Document requirement-to-test traceability.

### 11.7 Examples and Documentation

**Goal:** Make the executor easy to evaluate and extend.

**Tests first:**

- `phase11_qemu_tge_example_control_pipeline`
- `phase11_qemu_tge_example_rvv_pipeline`

**Implementation tasks:**

1. Add a small control pipeline example.
2. Add a simple RVV-capable pipeline example.
3. Document how to add a new scheduling policy.
4. Document how to add a new task graph test.

---

## 10. Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Executor becomes an RTOS | High | Keep non-goals explicit; no blocking or preemption in Phase 11 |
| Scheduler policies hide correctness bugs | High | Keep graph state transitions in executor core, not policies |
| SMP tests become nondeterministic | Medium | Validate final state first; add deterministic tie-breaks where observable |
| AMP design gets ahead of platform support | Medium | Store domain metadata early; defer remote execution until Phase 8 is ready |
| Functional-safety expectations become too broad | High | Say "safety-ready," not certified; maintain traceability and evidence |
| Overly large API too early | Medium | Start with single-hart DAG API and grow only when tests require it |

---

## 11. Dependencies

| Dependency | Source |
|------------|--------|
| CMake + CTest | Existing build/test infrastructure |
| QEMU and Spike | Existing simulator testbench |
| Phase 4 SMP primitives | Hart boot, barriers, spinlocks, atomics |
| Phase 5 RVV infrastructure | Capability and RVV workload tests |
| Phase 8 SystemC AMP platform | Future cross-domain executor validation |
| Existing console/UART/HTIF abstraction | Structured test output |

---

## 12. Open Questions and Potential Missing Requirements

The following top-level requirements should be decided before implementation or
explicitly deferred:

1. **Certification target:** Is the intended safety context ISO 26262, IEC
   61508, DO-178C, medical, industrial, or "safety-inspired only"?
2. **Maximum graph size:** What initial limits should be supported for tasks,
   edges, domains, and harts?
3. **Graph mutability:** Are graphs built once at startup, or should a later
   phase allow graph reset/reuse with different inputs?
4. **Failure policy:** On task failure, should the graph always stop, or should
   recovery tasks be supported?
5. **Deadline model:** Do we need hard deadlines, soft deadline classes, or just
   priorities for the first version?
6. **Data movement:** Are dependency edges control-only, or should data payloads
   be part of the executor API?
7. **Interrupt interaction:** Should interrupt handlers enqueue tasks, or should
   they only set flags observed by normal tasks?
8. **PMP / memory protection:** Should AMP safety partitions require PMP setup
   as part of executor initialization?
9. **Trace retention:** Should traces be kept only in RAM, emitted over UART, or
   exported through simulator-specific channels?
10. **Tool qualification:** Are any static-analysis or coverage tools mandatory
    for the safety-ready definition?
11. **Coding standard:** Should we formally target MISRA C, CERT C, or a local
    subset?
12. **Scheduling policy priority:** Which policies matter first: FIFO/priority,
    affinity, static cyclic executive, deadline-class, or work stealing?

---

## 13. Success Criteria

| Criterion | Measurement |
|-----------|-------------|
| Approval workflow exists | Design, roadmap phase, and task manifest are present and marked proposed |
| TDD compliance | Every implementation milestone lists CTest names before code tasks |
| Single-hart executor correctness | QEMU and Spike tests pass for static DAGs |
| Failure semantics | Invalid graph and task failure tests pass |
| Policy extensibility | FIFO and priority policies pass through the same policy interface |
| SMP readiness | QEMU and Spike SMP tests pass with shared ready queue |
| AMP readiness | Domain metadata tests pass before SystemC; remote dependency tests pass after Phase 8 |
| Safety readiness | Requirement-to-test traceability, watchdog hooks, bounded traces, and fault-injection tests exist |

---

*Next: See [ROADMAP.md](../ROADMAP.md) Phase 11 and
[`tasks/task-graph-executor.yaml`](../tasks/task-graph-executor.yaml).*
