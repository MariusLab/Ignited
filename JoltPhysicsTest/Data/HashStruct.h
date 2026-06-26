#pragma once

struct THashContainer
{
	static uint64 Generate(const FString& StringToHash)
	{
		return FHashedName(StringToHash).GetHash();
	}

	void Add(const int& InFrameId, const uint64& InHash)
	{
		checkf(!HashAndFrameId.Contains(InHash), TEXT("Hash already exists."));

		HashAndFrameId.Add(InHash, InFrameId);
	}

	bool Contains(const uint64& InHash) const
	{
		return HashAndFrameId.Contains(InHash);
	}

	void CleanupFrame(const int& InFrameId)
	{
		TArray<uint64> HashesToRemove;
		for (const auto& Pair : HashAndFrameId)
		{
			if (Pair.Value == InFrameId)
			{
				HashesToRemove.Add(Pair.Key);
			}
		}

		for (const auto& Hash : HashesToRemove)
		{
			HashAndFrameId.Remove(Hash);
		}
	}

private:
	TMap<uint64, int> HashAndFrameId;
};