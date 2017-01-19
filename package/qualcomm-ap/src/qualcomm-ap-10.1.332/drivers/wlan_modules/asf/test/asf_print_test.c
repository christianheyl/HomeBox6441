
#include <stdlib.h> /* NULL */

#include "asf_print.h"
#include "adf_os_types.h" /* adf_os_print */

#ifndef NAME
#define NAME "global"
#endif

#define NODE_VERB_LOW  10
#define NODE_VERB_HIGH 20

#define NODE_CATEGORY_FOO  0
#define NODE_CATEGORY_BAR  1

#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

static struct asf_print_bit_spec node_bit_specs[] = {
    {NODE_CATEGORY_FOO, "foo"},
    {NODE_CATEGORY_BAR, "bar"},
};

struct node_info {
    int id;
    struct asf_print_ctrl print_ctrl;
};

void node_print(struct node_info *node)
{
    asf_print(&node->print_ctrl, NODE_CATEGORY_FOO, NODE_VERB_LOW,
        "low-verbosity foo message from node %d\n", node->id);
    asf_print(&node->print_ctrl, NODE_CATEGORY_FOO, NODE_VERB_HIGH,
        "high-verbosity foo message from node %d\n", node->id);
    asf_print(&node->print_ctrl, NODE_CATEGORY_BAR, NODE_VERB_LOW,
        "low-verbosity bar message from node %d\n", node->id);
    asf_print(&node->print_ctrl, NODE_CATEGORY_BAR, NODE_VERB_HIGH,
        "high-verbosity bar message from node %d\n", node->id);
}

#define NUM_NODES 2
int main(void)
{
    struct node_info nodes[NUM_NODES];
    int i;

    for (i = 0; i < NUM_NODES; i++) {
        nodes[i].id = i;
        nodes[i].print_ctrl.num_bit_specs = ARRAY_LEN(node_bit_specs);
        nodes[i].print_ctrl.bit_specs = node_bit_specs;
        asf_print_ctrl_register(&nodes[i].print_ctrl);
    }

    adf_os_print(
        "\n--- printouts using the global print-control struct ---\n"
        "CHECK THAT PRINTOUTS ARE FILTERED BY CATEGORY AND VERBOSITY\n");
    asf_print(NULL, 0, 2, "debug 1\n");
    asf_print_verb_set_by_name("global", 2);
    asf_print(NULL, 0, 2, "debug 2\n");
    asf_print_mask_set_by_name(NAME, 0, 1);
    asf_print(NULL, 0, 2, "debug 3\n");
    asf_print(NULL, 1, 2, "debug 4\n");
    asf_print_mask_set_by_name(NAME, 1, 1);
    asf_print(NULL, 1, 2, "debug 5\n");
    asf_print(NULL, 1, 4, "debug 6\n");
    asf_print_verb_set_by_name(NAME, 4);
    asf_print(NULL, 1, 4, "debug 7\n");
    asf_print_mask_set_by_name(NAME, 1, 0);
    asf_print(NULL, 1, 2, "debug 8\n");

    if (NUM_NODES > 0) {
        /* node 0: foo and bar enabled */
        asf_print_mask_set(&nodes[0].print_ctrl, NODE_CATEGORY_FOO, 1);
        asf_print_mask_set(&nodes[0].print_ctrl, NODE_CATEGORY_BAR, 1);
        /* node 0: high-verbosity */
        nodes[0].print_ctrl.verb_threshold = NODE_VERB_HIGH;
    }
    if (NUM_NODES > 1) {
        /* node 1: foo enabled, bar disabled */
        asf_print_mask_set(&nodes[1].print_ctrl, NODE_CATEGORY_FOO, 1);
        asf_print_mask_set(&nodes[1].print_ctrl, NODE_CATEGORY_BAR, 0);
        /* node 1: low verbosity */
        nodes[1].print_ctrl.verb_threshold = NODE_VERB_LOW;
    }

    /* print out all nodes */
    adf_os_print(
        "\n--- printouts using the private print-control structs "
        "(settings 1) ---\n"
        "CHECK THAT PRINTOUT FILTERING BASED ON PRIVATE SPECS "
        "IS DONE INDEPENDENTLY\n");
    for (i = 0; i < NUM_NODES; i++) {
       node_print(&nodes[i]);
    }

    /* set all registered print-control structs to high verbosity */
    asf_print_verb_set_by_name(NULL, NODE_VERB_HIGH);

    /* set NODE_CATEGORY_BAR bit in all registered print-control structs */
    asf_print_mask_set_by_name(NULL, NODE_CATEGORY_BAR, 1);

    /* print out all nodes again */
    adf_os_print(
        "\n--- printouts using the private print-control structs "
        "(settings 2) ---\n"
        "CHECK THAT MULTIPLE PRINT-CONTROL STRUCTS CAN BE ADJUSTED "
        "WITH A SINGLE COMMAND\n");
    for (i = 0; i < NUM_NODES; i++) {
       node_print(&nodes[i]);
    }
    /* oops, that turned on category 1 in the global print-ctrl struct too */
    asf_print(NULL, 1, 2, "debug 9\n");


    /* clear NODE_CATEGORY_BAR bit in all registered print-control structs */
    asf_print_mask_set_by_name(NULL, NODE_CATEGORY_BAR, 0);

    /*
     * Now only set the category named "bar"
     * (in all registered print-control structs).
     * Similarly, clear the category named "foo".
     */
    asf_print_mask_set_by_bit_name(NULL, "bar", 1);
    asf_print_mask_set_by_bit_name(NULL, "foo", 0);

    /* print out all nodes again */
    adf_os_print(
        "\n--- printouts using the private print-control structs "
        "(settings 3) ---\n"
        "CHECK THAT PRINT CATEGORIES CAN BE SPECIFIED BY NAME "
        "RATHER THAN BIT-NUMBER\n");
    for (i = 0; i < NUM_NODES; i++) {
       node_print(&nodes[i]);
    }
    /* now the below printout shouldn't show up */
    asf_print(NULL, 1, 2, "debug 10\n");
}

