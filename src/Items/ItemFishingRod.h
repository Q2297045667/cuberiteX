
#pragma once

#include "../Entities/Floater.h"
#include "../Entities/Entity.h"
#include "../Item.h"
#include "../Root.h"





////////////////////////////////////////////////////////////////////////////////
// cFloaterCallback
class cFloaterCallback
{
public:
	cFloaterCallback(void) :
		m_CanPickup(false),
		m_AttachedMobID(cEntity::INVALID_ID)
	{
	}

	bool operator () (cEntity & a_Entity)
	{
		auto & Floater = static_cast<cFloater &>(a_Entity);
		m_CanPickup = Floater.CanPickup();
		m_Pos = Floater.GetPosition();
		m_BitePos = Floater.GetBitePos();
		m_AttachedMobID = Floater.GetAttachedMobID();
		Floater.Destroy();
		return true;
	}

	bool CanPickup(void)       const { return m_CanPickup; }
	bool IsAttached(void)      const { return (m_AttachedMobID != cEntity::INVALID_ID); }
	UInt32 GetAttachedMobID(void) const { return m_AttachedMobID; }
	Vector3d GetPos(void)      const { return m_Pos; }
	Vector3d GetBitePos(void)  const { return m_BitePos; }

protected:
	bool m_CanPickup;
	UInt32 m_AttachedMobID;
	Vector3d m_Pos;
	Vector3d m_BitePos;
} ;





class cItemFishingRodHandler final:
	public cItemHandler
{
	using Super = cItemHandler;

public:

	using Super::Super;





	virtual bool OnItemUse(
		cWorld * a_World,
		cPlayer * a_Player,
		cBlockPluginInterface & a_PluginInterface,
		const cItem & a_HeldItem,
		const Vector3i a_ClickedBlockPos,
		eBlockFace a_ClickedBlockFace
	) const override
	{
		if (a_ClickedBlockFace != BLOCK_FACE_NONE)
		{
			return false;
		}

		if (a_Player->IsFishing())
		{
			ReelIn(*a_World, *a_Player);
		}
		else
		{
			// Cast a hook:
			auto & Random = GetRandomProvider();
			auto CountDownTime = Random.RandInt(100, 900) - static_cast<int>(a_Player->GetEquippedItem().m_Enchantments.GetLevel(cEnchantments::enchLure) * 100);
			auto Floater = std::make_unique<cFloater>(
				a_Player->GetEyePosition(), a_Player->GetLookVector() * 15,
				a_Player->GetUniqueID(),
				CountDownTime
			);
			auto FloaterPtr = Floater.get();
			if (!FloaterPtr->Initialize(std::move(Floater), *a_World))
			{
				return false;
			}
			a_Player->SetIsFishing(true, FloaterPtr->GetUniqueID());
		}
		return true;
	}





	/** Reels back the fishing line, reeling any attached mob, or creating fished loot, or just breaking the fishing rod. */
	void ReelIn(cWorld & a_World, cPlayer & a_Player) const
	{
		cFloaterCallback FloaterInfo;
		a_World.DoWithEntityByID(a_Player.GetFloaterID(), FloaterInfo);
		a_Player.SetIsFishing(false);

		// If attached to an entity, reel it in:
		if (FloaterInfo.IsAttached())
		{
			ReelInEntity(a_World, a_Player, FloaterInfo.GetAttachedMobID());
			return;
		}

		// If loot can be caught, get it:
		if (FloaterInfo.CanPickup())
		{
			ReelInLoot(a_World, a_Player, FloaterInfo.GetBitePos());
			return;
		}

		// Empty fishing rod, just damage it:
		auto BlockType = a_World.GetBlock(FloaterInfo.GetPos() - Vector3d(0, 0.1, 0));
		if (cBlockAirHandler::IsBlockAir(BlockType) && (BlockType.Type() != BlockType::Water))
		{
			a_Player.UseEquippedItem(2);
		}
	}




	/** Reels back the entity, specified by the ID, and damages the fishing rod accordingly. */
	void ReelInEntity(cWorld & a_World, cPlayer & a_Player, UInt32 a_EntityID) const
	{
		auto PlayerPos = a_Player.GetPosition();
		a_World.DoWithEntityByID(a_EntityID, [=](cEntity & a_Entity)
			{
				auto Speed = PlayerPos - a_Entity.GetPosition();
				a_Entity.AddSpeed(Speed);
				return true;
			}
		);
		a_Player.UseEquippedItem(5);
	}





	void ReelInLoot(cWorld & a_World, cPlayer & a_Player, const Vector3d a_FloaterBitePos) const
	{
		auto LotSLevel = std::min(a_Player.GetEquippedItem().m_Enchantments.GetLevel(cEnchantments::enchLuckOfTheSea), 3u);

		// Chances for getting an item from the category for each level of Luck of the Sea (0 - 3)
		const int TreasureChances[] = {50, 71, 92, 113};  // 5% | 7.1% | 9.2% | 11.3%
		const int JunkChances[] = {100, 81, 61, 42};  // 10% | 8.1% | 6.1% | 4.2%

		cItems Drops;
		auto & Random = GetRandomProvider();
		int ItemCategory = Random.RandInt(999);
		if (ItemCategory < TreasureChances[LotSLevel])
		{
			switch (Random.RandInt(5))  // Each piece of treasure has an equal chance of 1 / 6
			{
				case 0:
				{
					cItem Bow(Item::Bow, 1, Random.RandInt<short>(50));
					Bow.EnchantByXPLevels(Random.RandInt(22U, 30U), GetRandomProvider());
					Drops.Add(Bow);
					break;
				}
				case 1:
				{
					cItem Book(Item::Book);
					Book.EnchantByXPLevels(30, GetRandomProvider());
					Drops.Add(Book);
					break;
				}
				case 2:
				{
					cItem Rod(Item::FishingRod, 1, Random.RandInt<short>(50));
					Rod.EnchantByXPLevels(Random.RandInt(22U, 30U), GetRandomProvider());
					Drops.Add(Rod);
					break;
				}
				case 3:
				{
					Drops.Add(cItem(Item::NameTag));
					break;
				}
				case 4:
				{
					Drops.Add(cItem(Item::Saddle));
					break;
				}
				case 5:
				{
					Drops.Add(cItem(Item::LilyPad));
					break;
				}
			}

			a_Player.GetStatistics().Custom[CustomStatistic::TreasureFished]++;
		}
		else if (ItemCategory < JunkChances[LotSLevel])
		{
			int Junk = Random.RandInt(82);
			if (Junk < 10)  // 10 / 83 chance of spawning a bowl
			{
				Drops.Add(cItem(Item::Bowl));
			}
			else if (Junk < 12)  // 2 / 83 chance of spawning a fishing rod
			{
				// Fishing Rods caught from the Junk category will be 10% .. 100% damaged, and always unenchanted.
				Drops.Add(cItem(Item::FishingRod, 1, Random.RandInt<short>(7, 65)));
			}
			else if (Junk < 22)  // 10 / 83 chance of spawning leather
			{
				Drops.Add(cItem(Item::Leather));
			}
			else if (Junk < 32)  // 10 / 83 chance of spawning leather boots
			{
				// Leather boots caught from the Junk category will be 10% .. 100% damaged, and always unenchanted.
				Drops.Add(cItem(Item::LeatherBoots, 1, Random.RandInt<short>(7, 66)));
			}
			else if (Junk < 42)  // 10 / 83 chance of spawning rotten flesh
			{
				Drops.Add(cItem(Item::RottenFlesh));
			}
			else if (Junk < 47)  // 5 / 83 chance of spawning a stick
			{
				Drops.Add(cItem(Item::Stick));
			}
			else if (Junk < 52)  // 5 / 83 chance of spawning string
			{
				Drops.Add(cItem(Item::String));
			}
			else if (Junk < 62)  // 10 / 83 chance of spawning a water bottle
			{
				Drops.Add(cItem(Item::Potion));
			}
			else if (Junk < 72)  // 10 / 83 chance of spawning a bone
			{
				Drops.Add(cItem(Item::Bone));
			}
			else if (Junk < 73)  // 1 / 83 chance of spawning an ink sac
			{
				Drops.Add(cItem(Item::BoneMeal));
			}
			else  // 10 / 83 chance of spawning a tripwire hook
			{
				Drops.Add(cItem(Item::TripwireHook));
			}

			a_Player.GetStatistics().Custom[CustomStatistic::JunkFished]++;
		}
		else
		{
			int FishType = Random.RandInt(99);
			if (FishType <= 1)  // Clownfish has a 2% chance of spawning
			{
				Drops.Add(cItem(Item::TropicalFish));
			}
			else if (FishType <= 12)  // Pufferfish has a 13% chance of spawning
			{
				Drops.Add(cItem(Item::Pufferfish));
			}
			else if (FishType <= 24)  // Raw salmon has a 25% chance of spawning
			{
				Drops.Add(cItem(Item::Salmon));
			}
			else  // Raw fish has a 60% chance of spawning
			{
				Drops.Add(cItem(Item::Cod));
			}

			a_Player.GetStatistics().Custom[CustomStatistic::FishCaught]++;
		}

		auto Experience = Random.RandInt(1, 6);

		// Check with plugins if this loot is acceptable:
		if (cRoot::Get()->GetPluginManager()->CallHookPlayerFishing(a_Player, Drops, Experience))
		{
			return;
		}

		// Spawn the loot and the experience orb:
		auto FloaterPos = a_FloaterBitePos.addedY(0.5);
		const float FISH_SPEED_MULT = 2.25f;
		Vector3d FlyDirection = (a_Player.GetEyePosition() - FloaterPos).addedY(1.0f) * FISH_SPEED_MULT;
		a_World.SpawnItemPickups(Drops, FloaterPos, FlyDirection);
		a_World.SpawnExperienceOrb(a_Player.GetPosition(), Experience);
		a_Player.UseEquippedItem(1);

		// Notify plugins
		cRoot::Get()->GetPluginManager()->CallHookPlayerFished(a_Player, Drops, Experience);
	}
} ;
