#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/arm/armv7m.h"
#include "hw/arm/boot.h"
#include "qom/object.h"
#include "hw/boards.h"

/* Main SYSCLK frequency in Hz */
#define SYSCLK_FRQ 320000000

#define TYPE_NXPS32K3_BOARD_BASE_MACHINE MACHINE_TYPE_NAME("NXPS32K3-board-base")
#define TYPE_NXPS32K3_BOARD_MACHINE MACHINE_TYPE_NAME("NXPS32K3-board")

DECLARE_OBJ_CHECKERS(NXPS32K3BoardMachineState, NXPS32K3BoardMachineClass,
        NXPS32K3_BOARD_MACHINE, TYPE_NXPS32K3_BOARD_MACHINE)

struct NXPS32K3BoardMachineState {
    MachineState parent_obj;
    ARMv7MState mcu;
    Clock *sysclk;
};
typedef struct NXPS32K3BoardMachineState NXPS32K3BoardMachineState;

struct NXPS32K3BoardMachineClass {
    MachineClass parent_class;
};

static void nxps32k3_board_init(MachineState *machine)
{
    //Make a specific MachineState out of the generic one
    NXPS32K3BoardMachineState* m_state = NXPS32K3_BOARD_MACHINE(machine);
    MemoryRegion *system_memory = get_system_memory();


    m_state->sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(m_state->sysclk, SYSCLK_FRQ);

    //We initialize the mocrocontroller that is part of the board
    object_initialize_child(OBJECT(m_state), "armv7m", &m_state->mcu, TYPE_ARMV7M);

    armv7m = DEVICE(&m_state->mcu);
    qdev_connect_clock_in(armv7m, "cpuclk", m_state->sysclk);
    qdev_prop_set_string(armv7m, "cpu-type", m_state->cpu_type);
    qdev_prop_set_bit(armv7m, "enable-bitband", true);
    object_property_set_link(OBJECT(&m_state->mcu), "memory",
                             OBJECT(system_memory), &error_abort);



    //And we connect it via QEMU's SYSBUS.
    sysbus_realize(SYS_BUS_DEVICE(&m_state->mcu), &error_fatal);

    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename,
                       0, 0x400000);
}

//Generic Objectc is passed by QEMU
static void nxpS32K3_board_class_init(ObjectClass *oc, void *data)
{
    //The generic machine class from object
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NXPS32K3 Example Board";
    mc->alias = "nxpS32K3-board";

    static const char * const valid_cpu_types[] = {
        ARM_CPU_TYPE_NAME("cortex-m7"),
        NULL
    };
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m7");
    mc->valid_cpu_types = valid_cpu_types;

    
    //Notice that we tell QEMU what function is used to initialize our board here.
    mc->init = nxps32k3_board_init;
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    // Our board does not have any media drive
    mc->no_floppy = 1;
    mc->no_cdrom = 1;
    //We also will not have threads
    mc->no_parallel = 1;
}

static const TypeInfo nxpS32K3_board_machine_types[] = {
        {
                                //Notice that this is the TYPE that we defined above.
                .name           = TYPE_NXPS32K3_BOARD_MACHINE,
                                //Our machine is a direct child of QEMU generic machine
                .parent         = TYPE_MACHINE,
                .instance_size  = sizeof(NXPS32K3BoardMachineState),
                .class_size     = sizeof(NXPS32K3BoardMachineClass),
                //We need to registers the class inti function 
                .class_init     = nxpS32K3_board_class_init,
        }
};
DEFINE_TYPES(nxpS32K3_board_machine_types)