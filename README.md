# Periodic Task Layer

This layer is written around the FreeRTOS kernel and allows to create a **Periodic Task Layer** handler.
This handler enable the user to generate periodic tasks and how they are handled in case of overrun (see policy description)


## Getting started

### Prerequisites

- `arm-none-eabi-gcc` for compiling/linking
- `make`
- `qemu` toolchain
- `python >= 3.11`
- `doxygen` for the documentation


### Clone and build

Clone this repository:

```bash
git clone --recurse-submodules https://baltig.polito.it/eos25/group3
cd group3
```

Build

```bash
make
```

## How to use it

Everytime the user want to instanciate a task, this must agree to introduce 2 macros in the code:

1. `PTL_CheckAbort()`, needed to check if that task is aborted and must be put after every expensive computation
2. `PTL_vTaskDelay()`, needed to implement delays and checks if that task is aborted


```c

void UserTask() {
    // Long computation
    PTL_CheckAbort();
    // Other computations
    // Introducing a delay
    PTL_vTaskDelay();
}
```

These task are then set in the following way

```c
int main() {
    
    // Configure and initialize the scheduler

    SchedulerConfig_t sch_cfg = { 
        .maxTasks = 3, 
        .xPolicy = PTL_OVERRUN_KILL, // Policy
        .xTraceEnabled = pdTRUE
    };

    PTL_Init( & sch_cfg );
    
    // Configure the tasks 

    PTL_TaskConfig task = {
        .pcTaskName        = "TASK",
        .PTL_JobFunction_t = UserTask,
        .usStack_size      = configMINIMAL_STACK_SIZE
        .xPriority         = tskIDLE_PRIORITY + 1,
        .xPeriod           = pdMS_TO_TICKS(120),
        .xDeadline         = pdMS_TO_TICKS(50),
        .xPhase            = pdMS_TO_TICKS(15),
        .pArg              = NULL
    };

    PTL_CreateTask(&task, NULL);

    // More and more tasks

    // ...

    // Start the scheduler

    PTL_Start(); // Start PTL
    
    for(;;);
    return 0;
}
```

## Configuration framework
To get schedulability analysis(Response Time Analysis) and generate starting template for projects.

Create a yamlfilename.yaml with inside a structure like the one below (tuning it according to your application):

```yaml
scheduler:
  policy: SKIP
  max_tasks: 8
  trace_enabled: true

tasks:
  - name: BrakeMonitor
    period_ms: 50
    deadline_ms: 50
    priority: 4
    wcet_ms: 5

  - name: EngineSensor
    period_ms: 100
    deadline_ms: 50
    priority: 3
    wcet_ms: 10

  - name: Dashboard
    period_ms: 200
    deadline_ms: 150
    priority: 2
    wcet_ms: 15

  - name: BrakePedalSim
    period_ms: 500
    priority: 1
    wcet_ms: 5
```

Then you can run the following commands:

```bash
make analyze yamlfilename
make generate yamlfilename
```


## Testing

To run all tests or specific test

```bash
make run_tests
make run-test_HRT_priority
```
As result you will find inside tests/log/ folder the .rpt files containing tracing and reports.
In addition, html files will be generated, they can be open inside your browser to display the graphs

## Demo
Inside demo/ folder you can find the files for a simple demo that mimic a false automotive environment application.

```bash
make
make run
```

## Documentation
Be sure to have `doxygen` installed in your system.
To generate documentation

```bash
make doc-gen
make doc-open
```


## License
Released under [MIT](docs/LICENSE)
