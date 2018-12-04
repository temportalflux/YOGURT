// Fill out your copyright notice in the Description page of Project Settings.

#include "QuadPackingSolver.h"

QuadNode::QuadNode()
{
	this->mPosition = FIntPoint::ZeroValue;
	this->mSize = FIntPoint::ZeroValue;
	this->mIsFilled = false;
	this->mFilledSize = FIntPoint::ZeroValue;
	this->mpChildRight = nullptr;
	this->mpChildBelow = nullptr;
}

QuadNode::~QuadNode()
{
	if (this->mpChildRight != nullptr) delete this->mpChildRight;
	if (this->mpChildBelow != nullptr) delete this->mpChildBelow;
}

bool QuadNode::PackQuad(FIntPoint quad, FIntPoint &uvCoord)
{
	if (this->mIsFilled)
	{
		return (this->mpChildRight != nullptr && this->mpChildRight->PackQuad(quad, uvCoord))
			|| (this->mpChildBelow != nullptr && this->mpChildBelow->PackQuad(quad, uvCoord));
	}
	else if (quad.X <= this->mSize.X && quad.Y <= this->mSize.Y)
	{
		this->mIsFilled = true;
		uvCoord = this->mPosition;
		this->mFilledSize = quad;

		FVector2D sizeRemaining = this->mSize - quad;

		if (sizeRemaining.X > 0)
		{
			this->mpChildRight = new QuadNode();
			this->mpChildRight->mPosition = this->mPosition + FIntPoint(quad.X, 0);
			this->mpChildRight->mSize = FIntPoint(sizeRemaining.X, quad.Y);
		}

		if (sizeRemaining.Y > 0)
		{
			this->mpChildBelow = new QuadNode();
			this->mpChildBelow->mPosition = this->mPosition + FIntPoint(0, quad.Y);
			this->mpChildBelow->mSize = FIntPoint(this->mSize.X, sizeRemaining.Y);
		}

		return true;
	}
	else
	{
		return false;
	}
}

// assumes that we do contain the point
QuadNode* QuadNode::GetSubnodeAt(FIntPoint position)
{
	// Best option: the right child has it
	if (this->mpChildRight != nullptr && this->mpChildRight->ContainsPoint(position))
	{
		return this->mpChildRight->GetSubnodeAt(position);
	}
	// Second best: the below child has it
	else if (this->mpChildBelow != nullptr && this->mpChildBelow->ContainsPoint(position))
	{
		return this->mpChildBelow->GetSubnodeAt(position);
	}
	// Default: It is in the filled area
	else if (this->ContainsPointInFilledArea(position))
	{
		return this;
	}
	// Bad: This function was called but this node doesnt actually contain the point
	else
	{
		return nullptr;
	}
}

bool QuadNode::ContainsPoint(FIntPoint position)
{
	return this->ContainsPointInArea(position, this->mSize);
}

bool QuadNode::ContainsPointInFilledArea(FIntPoint position)
{
	return this->ContainsPointInArea(position, this->mFilledSize);
}

bool QuadNode::ContainsPointInArea(FIntPoint position, FIntPoint size)
{
	FVector2D posRelative = FVector2D(position - this->mPosition);
	return posRelative > FVector2D::ZeroVector && posRelative < size;
}

QuadPackingSolver::QuadPackingSolver()
{
	this->mpRootNode = nullptr;
}

QuadPackingSolver::~QuadPackingSolver()
{
	if (this->mpRootNode.IsValid()) this->mpRootNode.Reset();
}

void QuadPackingSolver::SetMaxSize(FIntPoint size)
{
	this->mMaxSize = size;
	this->mpRootNode = MakeShareable(new QuadNode());
	this->mpRootNode->mSize = size;
}

TSharedPtr<QuadNode> QuadPackingSolver::GetRootNode()
{
	return this->mpRootNode;
}

bool QuadPackingSolver::TryPack(FIntPoint quadSize, FIntPoint &successfulUvCoordinate)
{
	return this->mpRootNode->PackQuad(quadSize, successfulUvCoordinate);
}
