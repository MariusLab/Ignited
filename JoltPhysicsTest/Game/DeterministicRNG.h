#pragma once

class DeterministicRNG
{
public:
	DeterministicRNG() { State = 0; }
	DeterministicRNG(uint64_t InSeed) { State = InSeed; OriginalSeed = InSeed; }

	void Seed(const uint64_t InSeed)
	{
		State = InSeed;
		OriginalSeed = InSeed;
	}

	uint64_t NextInt()
	{
		uint64_t z = ( State += 0x9e3779b97f4a7c15ull );
		z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
		z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebull;
			
		return z ^ ( z >> 31 );
	}

	int32_t NextIntInRange(int32_t Min, int32_t Max)
	{
		if (Min > Max) std::swap(Min, Max);

		uint64_t Range = (uint64_t)Max - (uint64_t)Min + 1ull;
		uint64_t Rand = NextInt();

		// Map uniformly into [0, Range)
		uint64_t Value = Rand % Range;

		return (int32_t)(Min + Value);
	}

	// Returns a normalized float 0.f - 1.f
	float NextFloat()
	{
		// Get a 64-bit random integer
		uint64_t Rand = NextInt();

		// Keep only the top 24 bits (fits in float precision)
		return (Rand >> 40) * (1.0f / (1ull << 24));
	}

	void RestoreState(const uint64_t& StateToLoad)
	{
		State = StateToLoad;
	}

	uint64_t GetState() const
	{
		return State;
	}

	uint64_t GetOriginalSeed() const
	{
		return OriginalSeed;
	}

private:
	uint64_t State;
	uint64_t OriginalSeed = 0;
};
