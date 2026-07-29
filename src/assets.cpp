enum TextureType {
  TEXTURE_PLAYER,
  TEXTURE_BLOCK,
  TEXTURE_BLOCK_ITEM,
  TEXTURE_STORAGE,
  TEXTURE_STORAGE_ITEM,
  TEXTURE_CONVEYOR,
  TEXTURE_CONVEYOR_ITEM,
  TEXTURE_WORLD_TUNNEL,
  TEXTURE_MESSAGE_SENDER,
  TEXTURE_MESSAGE_RECEIVER,
  TEXTURE_ASSEMBLER,
  TEXTURE_ASSEMBLER_ITEM,
  TEXTURE_COPPER_ITEM,
  TEXTURE_PLASTIC_ITEM,
  TEXTURE_ALUMINIUM_ITEM,
  TEXTURE_COPPER_WIRE_ITEM,
  TEXTURE_BASIC_CIRCUIT_BOARD_ITEM,
  TEXTURE_ANTENNA_ITEM,
  TEXTURE_COMMUNICATION_COMPONENT_ITEM,

  TEXTURE_COUNT,
};

static std::string_view get_texture_path(TextureType texture) {
  switch (texture) {
    case TEXTURE_PLAYER:
      return "assets/player.png";
    case TEXTURE_BLOCK:
      return "assets/block.png";
    case TEXTURE_BLOCK_ITEM:
      return "assets/block_item.png";
    case TEXTURE_STORAGE:
      return "assets/storage.png";
    case TEXTURE_STORAGE_ITEM:
      return "assets/storage_item.png";
    case TEXTURE_CONVEYOR:
      return "assets/conveyor.png";
    case TEXTURE_CONVEYOR_ITEM:
      return "assets/conveyor_item.png";
    case TEXTURE_WORLD_TUNNEL:
      return "assets/world_tunnel.png";
    case TEXTURE_MESSAGE_SENDER:
      return "assets/message_sender.png";
    case TEXTURE_MESSAGE_RECEIVER:
      return "assets/message_receiver.png";
    case TEXTURE_ASSEMBLER:
      return "assets/assembler.png";
    case TEXTURE_ASSEMBLER_ITEM:
      return "assets/assembler_item.png";
    case TEXTURE_COPPER_ITEM:
      return "assets/copper_item.png";
    case TEXTURE_PLASTIC_ITEM:
      return "assets/plastic_item.png";
    case TEXTURE_ALUMINIUM_ITEM:
      return "assets/aluminium_item.png";
    case TEXTURE_COPPER_WIRE_ITEM:
      return "assets/copper_wire_item.png";
    case TEXTURE_BASIC_CIRCUIT_BOARD_ITEM:
      return "assets/basic_circuit_board_item.png";
    case TEXTURE_ANTENNA_ITEM:
      return "assets/antenna_item.png";
    case TEXTURE_COMMUNICATION_COMPONENT_ITEM:
      return "assets/communication_component_item.png";
    case TEXTURE_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}

struct AssetManager {
  std::array<Texture2D, TEXTURE_COUNT> textures{};
};

void load_textures(AssetManager& assets) {
  for (u32 i = 0; i < TEXTURE_COUNT; ++i) {
    assets.textures[i] = LoadTexture(get_texture_path(TextureType(i)).data());
  }
}
