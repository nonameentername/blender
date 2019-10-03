
#include "node_composite_util.h"

static bNodeSocketTemplate cmp_node_gmic_in[] = {
	{ SOCK_RGBA,  1, N_("Image"),      1.0f, 1.0f, 1.0f, 1.0f },
	{ SOCK_FLOAT, 1, N_("$arg1"),      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, FLT_MAX },
	{ SOCK_FLOAT, 1, N_("$arg2"),      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, FLT_MAX },
	{ SOCK_FLOAT, 1, N_("$arg3"),      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, FLT_MAX },
	{ SOCK_FLOAT, 1, N_("$arg4"),      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, FLT_MAX },
	{ SOCK_FLOAT, 1, N_("$arg5"),      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, FLT_MAX },
	{ -1, 0, "" }
};

static bNodeSocketTemplate cmp_node_gmic_out[] = {
	{ SOCK_RGBA, 0, N_("Image") },
	{ -1, 0, "" }
};

static void node_composit_init_gmic(bNodeTree *UNUSED(ntree), bNode *node)
{
	NodeGmic *data = MEM_callocN(sizeof(NodeGmic), "gmic node");

	data->quality = 0.5f;
	data->flag = CMP_NODE_GMIC_NORMALIZE;
	memset(data->command, 0, sizeof(data->command));

	node->storage = data;
}

void register_node_type_cmp_gmic(void)
{
	static bNodeType ntype;

	cmp_node_type_base(&ntype, CMP_NODE_GMIC, "G'MIC", NODE_CLASS_OP_FILTER, 0);
	node_type_socket_templates(&ntype, cmp_node_gmic_in, cmp_node_gmic_out);
	node_type_init(&ntype, node_composit_init_gmic);
	node_type_storage(&ntype, "NodeGmic", node_free_standard_storage, node_copy_standard_storage);

	nodeRegisterType(&ntype);
}
