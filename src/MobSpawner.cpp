
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "MobSpawner.h"
#include "BlockInfo.h"
#include "Mobs/IncludeAllMonsters.h"
#include "World.h"
#include "Blocks/BlockLeaves.h"





cMobSpawner::cMobSpawner(cMonster::eFamily a_MonsterFamily, const std::set<eMonsterType>& a_AllowedTypes) :
	m_MonsterFamily(a_MonsterFamily),
	m_NewPack(true),
	m_MobType(mtInvalidType)
{
	for (std::set<eMonsterType>::const_iterator itr = a_AllowedTypes.begin(); itr != a_AllowedTypes.end(); ++itr)
	{
		if (cMonster::FamilyFromType(*itr) == a_MonsterFamily)
		{
			m_AllowedTypes.insert(*itr);
		}
	}
}





bool cMobSpawner::CheckPackCenter(BlockState a_Block)
{
	// Packs of non-water mobs can only be centered on an air block
	// Packs of water mobs can only be centered on a water block
	if (m_MonsterFamily == cMonster::mfWater)
	{
		return (a_Block.Type() == BlockType::Water);
	}
	else
	{
		return IsBlockAir(a_Block);
	}
}





eMonsterType cMobSpawner::ChooseMobType(EMCSBiome a_Biome)
{
	std::vector<eMonsterType> AllowedMobs;

	for (eMonsterType MobType : GetAllowedMobTypes(a_Biome))
	{
		auto itr = m_AllowedTypes.find(MobType);
		if (itr != m_AllowedTypes.end())
		{
			AllowedMobs.push_back(MobType);
		}
	}

	// Pick a random mob from the options:
	if (AllowedMobs.empty())
	{
		return mtInvalidType;
	}

	return AllowedMobs[GetRandomProvider().RandInt(AllowedMobs.size() - 1)];
}





bool cMobSpawner::CanSpawnHere(cChunk * a_Chunk, Vector3i a_RelPos, eMonsterType a_MobType, EMCSBiome a_Biome, bool a_DisableSolidBelowCheck)
{
	if ((a_RelPos.y >= cChunkDef::Height - 1) || (a_RelPos.y <= 0))
	{
		return false;
	}

	if (cChunkDef::IsValidHeight(a_RelPos.addedY(-1)) && (a_Chunk->GetBlock(a_RelPos.addedY(-1)).Type() == BlockType::Bedrock))
	{
		return false;   // Make sure mobs do not spawn on bedrock.
	}

	auto & Random = GetRandomProvider();
	auto TargetBlock = a_Chunk->GetBlock(a_RelPos);

	auto BlockLight = a_Chunk->GetBlockLight(a_RelPos);
	auto SkyLight = a_Chunk->GetSkyLight(a_RelPos);
	auto BlockAbove = a_Chunk->GetBlock(a_RelPos.addedY(1));
	auto BlockBelow = a_Chunk->GetBlock(a_RelPos.addedY(-1));

	SkyLight = a_Chunk->GetTimeAlteredLight(SkyLight);

	switch (a_MobType)
	{
		case mtBat:
		{
			return
			(
				(a_RelPos.y <= 63) &&
				(BlockLight <= 4) &&
				(SkyLight <= 4) &&
				IsBlockAir(TargetBlock) &&
				(!cBlockInfo::IsTransparent(BlockAbove))
			);
		}

		case mtBlaze:
		{
			return
			(
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck)) &&
				(Random.RandBool())
			);
		}

		case mtCaveSpider:
		{
			return
			(
				IsBlockAir(TargetBlock) &&
				((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck)) &&
				(SkyLight <= 7) &&
				(BlockLight <= 7) &&
				(Random.RandBool())
			);
		}

		case mtChicken:
		case mtCow:
		case mtPig:
		case mtHorse:
		case mtRabbit:
		case mtSheep:
		{
			return
			(
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				(BlockBelow.Type() == BlockType::GrassBlock) &&
				(SkyLight >= 9)
			);
		}

		case mtCreeper:
		case mtSkeleton:
		case mtZombie:
		{
			return
			(
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck)) &&
				(SkyLight <= 7) &&
				(BlockLight <= 7) &&
				(Random.RandBool())
			);
		}

		case mtEnderman:
		{
			if (a_RelPos.y < 250)
			{
				auto BlockTop = a_Chunk->GetBlock(a_RelPos.addedY(2));
				if (IsBlockAir(BlockTop))
				{
					BlockTop = a_Chunk->GetBlock(a_RelPos.addedY(3));
					return
					(
						IsBlockAir(TargetBlock) &&
						IsBlockAir(BlockAbove) &&
						IsBlockAir(BlockTop) &&
						((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck)) &&
						(SkyLight <= 7) &&
						(BlockLight <= 7)
					);
				}
			}
			break;
		}

		case mtGhast:
		{
			return
			(
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				(Random.RandBool(0.01))
			);
		}

		case mtGuardian:
		{
			return
			(
				(TargetBlock.Type() == BlockType::Water) &&
				(BlockBelow.Type() == BlockType::Water) &&
				(a_RelPos.y >= 45) &&
				(a_RelPos.y <= 62)
			);
		}

		case mtMagmaCube:
		case mtSlime:
		{
			const int AMOUNT_MOON_PHASES = 8;
			auto maxLight = Random.RandInt(0, 7);
			auto moonPhaseNumber = static_cast<int>(std::floor(a_Chunk->GetWorld()->GetWorldAge().count() / 24000)) % AMOUNT_MOON_PHASES;
			auto moonThreshold = static_cast<float>(std::abs(moonPhaseNumber - (AMOUNT_MOON_PHASES / 2)) / (AMOUNT_MOON_PHASES / 2));
			return
			(
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				(
					(!cBlockInfo::IsTransparent(BlockBelow)) ||
					(a_DisableSolidBelowCheck)) &&
				(
					(
						(a_RelPos.y <= 40) &&
						a_Chunk->IsSlimeChunk()
					) ||
					(
						(a_Biome == biSwampland) &&
						(a_RelPos.y >= 50) &&
						(a_RelPos.y <= 70) &&
						(SkyLight <= maxLight) &&
						(BlockLight <= maxLight) &&
						(Random.RandBool(moonThreshold)) &&
						(Random.RandBool(0.5))
					)
				)
			);
		}

		case mtMooshroom:
		{
			return
				(
					IsBlockAir(TargetBlock) &&
					IsBlockAir(BlockAbove) &&
					(BlockBelow.Type() == BlockType::Mycelium) &&
				(
					(a_Biome == biMushroomShore) ||
					(a_Biome == biMushroomIsland)
				)
			);
		}

		case mtOcelot:
		{
			return (
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				(
					(BlockBelow.Type() == BlockType::GrassBlock) || cBlockLeavesHandler::IsBlockLeaves(BlockBelow)
				) &&
				(a_RelPos.y >= 62) &&
				(Random.RandBool(2.0 / 3.0))
			);
		}

		case mtSpider:
		{
			bool CanSpawn = true;
			bool HasFloor = false;
			for (int x = 0; x < 2; ++x)
			{
				for (int z = 0; z < 2; ++z)
				{
					CanSpawn = a_Chunk->UnboundedRelGetBlock(a_RelPos.addedXZ(x, z), TargetBlock);
					CanSpawn = CanSpawn && IsBlockAir(TargetBlock);
					if (!CanSpawn)
					{
						return false;
					}
					HasFloor = (
						HasFloor ||
						(
							a_Chunk->UnboundedRelGetBlock(a_RelPos + Vector3i(x, -1, z), TargetBlock) &&
							!cBlockInfo::IsTransparent(TargetBlock)
						)
					);
				}
			}
			return CanSpawn && HasFloor && (SkyLight <= 7) && (BlockLight <= 7);
		}

		case mtSquid:
		{
			return (
				(TargetBlock.Type() == BlockType::Water) &&
				(a_RelPos.y >= 45) &&
				(a_RelPos.y <= 62)
			);
		}

		case mtWitherSkeleton:
		{
			return (
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck)) &&
				(SkyLight <= 7) &&
				(BlockLight <= 7) &&
				(Random.RandBool(0.6))
			);
		}

		case mtWolf:
		{
			return (
				(TargetBlock.Type() == BlockType::GrassBlock) &&
				IsBlockAir(BlockAbove) &&
				(
					(a_Biome == biColdTaiga) ||
					(a_Biome == biColdTaigaHills) ||
					(a_Biome == biColdTaigaM) ||
					(a_Biome == biForest) ||
					(a_Biome == biTaiga) ||
					(a_Biome == biTaigaHills) ||
					(a_Biome == biTaigaM) ||
					(a_Biome == biMegaTaiga) ||
					(a_Biome == biMegaTaigaHills)
				)
			);
		}

		case mtZombiePigman:
		{
			return (
				IsBlockAir(TargetBlock) &&
				IsBlockAir(BlockAbove) &&
				((!cBlockInfo::IsTransparent(BlockBelow)) || (a_DisableSolidBelowCheck))
			);
		}

		default:
		{
			LOGD("MG TODO: Write spawning rule for mob type %d", a_MobType);
			return false;
		}
	}
	return false;
}





std::set<eMonsterType> cMobSpawner::GetAllowedMobTypes(EMCSBiome a_Biome)
{
	std::set<eMonsterType> ListOfSpawnables;
	// Check biomes first to get a list of animals
	switch (a_Biome)
	{
		// Mooshroom only - no other mobs on mushroom islands
		case biMushroomIsland:
		case biMushroomShore:
		{
			ListOfSpawnables.insert(mtMooshroom);
			return ListOfSpawnables;
		}

		// Add Squid in ocean and river biomes
		case biOcean:
		case biFrozenOcean:
		case biFrozenRiver:
		case biRiver:
		case biDeepOcean:
		{
			ListOfSpawnables.insert(mtGuardian);
			break;
		}

		// Add ocelots in jungle biomes
		case biJungle:
		case biJungleHills:
		case biJungleEdge:
		case biJungleM:
		case biJungleEdgeM:
		{
			ListOfSpawnables.insert(mtOcelot);
			break;
		}

		// Add horses in plains-like biomes
		case biPlains:
		case biSunflowerPlains:
		case biSavanna:
		case biSavannaPlateau:
		case biSavannaM:
		case biSavannaPlateauM:
		{
			ListOfSpawnables.insert(mtHorse);
			break;
		}

		// Add wolves in forest biomes
		case biForest:
		{
			ListOfSpawnables.insert(mtWolf);
			break;
		}

		// Add wolves and rabbits in all taiga biomes
		case biColdTaiga:
		case biColdTaigaM:
		case biColdTaigaHills:
		case biTaiga:
		case biTaigaHills:
		case biTaigaM:
		case biMegaTaiga:
		case biMegaTaigaHills:
		{
			ListOfSpawnables.insert(mtWolf);
			ListOfSpawnables.insert(mtRabbit);
			break;
		}

		// Add rabbits in desert and flower forest biomes
		case biDesert:
		case biDesertHills:
		case biDesertM:
		case biFlowerForest:
		{
			ListOfSpawnables.insert(mtRabbit);
			break;
		}

		// Nothing special about this biome
		default:
		{
			break;
		}
	}

	// Overworld
	if (
		(a_Biome != biDesertHills) &&
		(a_Biome != biDesert) &&
		(a_Biome != biDesertM) &&
		(a_Biome != biBeach) &&
		(a_Biome != biOcean) &&
		(a_Biome != biDeepOcean))
	{
		ListOfSpawnables.insert(mtSheep);
		ListOfSpawnables.insert(mtPig);
		ListOfSpawnables.insert(mtCow);
		ListOfSpawnables.insert(mtChicken);
		ListOfSpawnables.insert(mtEnderman);
		ListOfSpawnables.insert(mtSlime);
	}

	ListOfSpawnables.insert(mtBat);
	ListOfSpawnables.insert(mtSpider);
	ListOfSpawnables.insert(mtZombie);
	ListOfSpawnables.insert(mtSkeleton);
	ListOfSpawnables.insert(mtCreeper);
	ListOfSpawnables.insert(mtSquid);

	// Nether
	ListOfSpawnables.insert(mtBlaze);
	ListOfSpawnables.insert(mtGhast);
	ListOfSpawnables.insert(mtMagmaCube);
	ListOfSpawnables.insert(mtWitherSkeleton);
	ListOfSpawnables.insert(mtZombiePigman);

	return ListOfSpawnables;
}





cMonster * cMobSpawner::TryToSpawnHere(cChunk * a_Chunk, Vector3i a_RelPos, EMCSBiome a_Biome, int & a_MaxPackSize)
{
	// If too close to any player, don't spawn anything
	auto AbsPos = a_Chunk->RelativeToAbsolute(a_RelPos);
	static const double RangeLimit = 24;
	if (
		a_Chunk->GetWorld()->DoWithNearestPlayer(
			AbsPos,
			RangeLimit,
			[](cPlayer & a_Player)
			{
				return true;
			}
		)
	)
	{
		return nullptr;
	}

	if (m_NewPack)
	{
		m_MobType = ChooseMobType(a_Biome);
		if (m_MobType == mtInvalidType)
		{
			return nullptr;
		}
		if (m_MobType == mtWitherSkeleton)
		{
			a_MaxPackSize = 5;
		}
		else if (m_MobType == mtWolf)
		{
			a_MaxPackSize = 8;
		}
		else if (m_MobType == mtGhast)
		{
			a_MaxPackSize = 1;
		}
		m_NewPack = false;
	}

	if ((m_AllowedTypes.find(m_MobType) != m_AllowedTypes.end()) && CanSpawnHere(a_Chunk, a_RelPos, m_MobType, a_Biome))
	{
		auto NewMob = cMonster::NewMonsterFromType(m_MobType);
		auto NewMobPtr = NewMob.get();
		if (NewMob)
		{
			m_Spawned.push_back(std::move(NewMob));
		}
		return NewMobPtr;
	}

	return nullptr;
}





void cMobSpawner::NewPack()
{
	m_NewPack = true;
}





bool cMobSpawner::CanSpawnAnything(void)
{
	return !m_AllowedTypes.empty();
}




