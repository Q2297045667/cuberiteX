
#pragma once

#include "BlockHandler.h"
#include "../Entities/Player.h"
#include "../ClientHandle.h"




class cBlockCauldronHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	virtual cItems ConvertToPickups(BlockState a_Block, const cEntity * a_Digger, const cItem * a_Tool) const override
	{
		return cItem(Item::Cauldron);
	}





	virtual bool OnUse(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cPlayer & a_Player,
		const Vector3i a_BlockPos,
		eBlockFace a_BlockFace,
		const Vector3i a_CursorPos
	) const override
	{
		auto FillState = Block::Cauldron::Level(a_ChunkInterface.GetBlock(a_BlockPos));
		auto EquippedItem = a_Player.GetEquippedItem();

		switch (EquippedItem.m_ItemType)
		{
			case E_ITEM_BUCKET:
			{
				if (FillState == 3)
				{
					a_ChunkInterface.FastSetBlock(a_BlockPos, Block::Cauldron::Cauldron(0));
					// Give new bucket, filled with fluid when the gamemode is not creative:
					if (!a_Player.IsGameModeCreative())
					{
						a_Player.ReplaceOneEquippedItemTossRest(cItem(Item::WaterBucket));
					}
				}
				break;
			}
			case E_ITEM_WATER_BUCKET:
			{
				if (FillState < 3)
				{
					a_ChunkInterface.FastSetBlock(a_BlockPos, Block::Cauldron::Cauldron(3));
					// Give empty bucket back when the gamemode is not creative:
					if (!a_Player.IsGameModeCreative())
					{
						a_Player.ReplaceOneEquippedItemTossRest(cItem(Item::Bucket));
					}
					a_Player.GetStatManager().AddValue(Statistic::FillCauldron);
				}
				break;
			}
			case E_ITEM_GLASS_BOTTLE:
			{
				if (FillState > 0)
				{
					a_ChunkInterface.FastSetBlock(a_BlockPos, Block::Cauldron::Cauldron(--FillState));
					// Give new potion when the gamemode is not creative:
					if (!a_Player.IsGameModeCreative())
					{
						a_Player.ReplaceOneEquippedItemTossRest(cItem(Item::Potion));
					}
					a_Player.GetStatManager().AddValue(Statistic::UseCauldron);
				}
				break;
			}
			case E_ITEM_POTION:
			{
				// Refill cauldron with water bottles.
				if ((FillState < 3) && (EquippedItem.m_ItemDamage == 0))
				{
					a_ChunkInterface.FastSetBlock(Vector3i(a_BlockPos), Block::Cauldron::Cauldron(++FillState));
					// Give back an empty bottle when the gamemode is not creative:
					if (!a_Player.IsGameModeCreative())
					{
						a_Player.ReplaceOneEquippedItemTossRest(cItem(Item::GlassBottle));
					}
				}
				break;
			}
			case E_ITEM_LEATHER_BOOTS:
			case E_ITEM_LEATHER_CAP:
			case E_ITEM_LEATHER_PANTS:
			case E_ITEM_LEATHER_TUNIC:
			{
				// Resets any color to default:
				if ((FillState > 0) && ((EquippedItem.m_ItemColor.GetRed() != 255) || (EquippedItem.m_ItemColor.GetBlue() != 255) || (EquippedItem.m_ItemColor.GetGreen() != 255)))
				{
					a_ChunkInterface.FastSetBlock(a_BlockPos, Block::Cauldron::Cauldron(--FillState));
					auto NewItem = cItem(EquippedItem);
					NewItem.m_ItemColor.Clear();
					a_Player.ReplaceOneEquippedItemTossRest(NewItem);
				}
				break;
			}
			case E_ITEM_BLACK_SHULKER_BOX:
			case E_ITEM_BLUE_SHULKER_BOX:
			case E_ITEM_BROWN_SHULKER_BOX:
			case E_ITEM_CYAN_SHULKER_BOX:
			case E_ITEM_GRAY_SHULKER_BOX:
			case E_ITEM_GREEN_SHULKER_BOX:
			case E_ITEM_LIGHT_BLUE_SHULKER_BOX:
			case E_ITEM_LIGHT_GRAY_SHULKER_BOX:
			case E_ITEM_LIME_SHULKER_BOX:
			case E_ITEM_MAGENTA_SHULKER_BOX:
			case E_ITEM_ORANGE_SHULKER_BOX:
			case E_ITEM_PINK_SHULKER_BOX:
			case E_ITEM_RED_SHULKER_BOX:
			case E_ITEM_YELLOW_SHULKER_BOX:
			{
				// Resets shulker box color.

				// TODO: When there is an actual default shulker box add the appropriate changes here! - 19.09.2020 - 12xx12
				if (FillState == 0)
				{
					// The cauldron is empty:
					break;
				}

				// Proceed with normal cleaning:
				a_ChunkInterface.FastSetBlock(a_BlockPos, Block::Cauldron::Cauldron(--FillState));
				auto NewShulker = cItem(EquippedItem);
				NewShulker.m_ItemType = E_ITEM_PURPLE_SHULKER_BOX;
				a_Player.ReplaceOneEquippedItemTossRest(NewShulker);
				break;
			}
		}

		if (!ItemHandler(EquippedItem.m_ItemType)->IsPlaceable())
		{
			// Item not placeable in the first place, our work is done:
			return true;
		}

		// This is a workaround for versions < 1.13, where rclking a cauldron with a block, places a block.
		// Using cauldrons with blocks was added in 1.13 as part of shulker cleaning.
		const auto ResendPosition = AddFaceDirection(a_BlockPos, a_BlockFace);
		a_Player.GetClientHandle()->SendBlockChange(
			ResendPosition.x, ResendPosition.y, ResendPosition.z,
			a_ChunkInterface.GetBlock(ResendPosition)
		);

		return true;
	}





	virtual bool IsUseable() const override
	{
		return true;
	}





	virtual void OnUpdate(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cBlockPluginInterface & a_PluginInterface,
		cChunk & a_Chunk,
		const Vector3i a_RelPos
	) const override
	{
		auto WorldPos = a_Chunk.RelativeToAbsolute(a_RelPos);
		if (!a_WorldInterface.IsWeatherWetAtXYZ(WorldPos.addedY(1)))
		{
			// It's not raining at our current location or we do not have a direct view of the sky
			return;
		}

		auto FillState = Block::Cauldron::Level(a_Chunk.GetBlock(a_RelPos));
		if (FillState < 3)
		{
			a_Chunk.FastSetBlock(a_RelPos, Block::Cauldron::Cauldron(FillState + 1));
		}
	}





	virtual ColourID GetMapBaseColourID() const override
	{
		return 21;
	}
} ;




