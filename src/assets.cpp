#include "assets.h"

#include "core.h"

std::string_view get_texture_path(TextureType texture) {
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
    case TEXTURE_CIRCUIT_BOARD_ITEM:
      return "assets/circuit_board_item.png";
    case TEXTURE_ANTENNA_ITEM:
      return "assets/antenna_item.png";
    case TEXTURE_COMMUNICATION_COMPONENT_ITEM:
      return "assets/communication_component_item.png";
    case TEXTURE_WIRE_BUNDLE_ITEM:
      return "assets/wire_bundle_item.png";
    case TEXTURE_COGWHEEL_ITEM:
      return "assets/cogwheel_item.png";
    case TEXTURE_SPARE_PARTS_ITEM:
      return "assets/spare_parts_item.png";
    case TEXTURE_BRUSH_ITEM:
      return "assets/brush_item.png";
    case TEXTURE_LUBRICANT_CAN_ITEM:
      return "assets/lubricant_can_item.png";
    case TEXTURE_OIL_CANISTER_ITEM:
      return "assets/oil_canister_item.png";
    case TEXTURE_CALIBRATOR_ITEM:
      return "assets/calibrator_item.png";
    case TEXTURE_SILICON_WAFER_ITEM:
      return "assets/silicon_wafer_item.png";
    case TEXTURE_BLANK_BOARD_ITEM:
      return "assets/blank_board_item.png";
    case TEXTURE_TRANSISTOR_ITEM:
      return "assets/transistor_item.png";
    case TEXTURE_CAPACITOR_ITEM:
      return "assets/capacitor_item.png";
    case TEXTURE_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}

void load_textures(AssetManager& assets) {
  for (u32 i = 0; i < TEXTURE_COUNT; ++i) {
    assets.textures[i] = LoadTexture(get_texture_path(TextureType(i)).data());
  }
}
