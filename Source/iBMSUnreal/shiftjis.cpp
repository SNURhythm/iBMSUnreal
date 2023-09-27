#include "shiftjis.h"

// Sets default values
Ashiftjis::Ashiftjis()
{
	// Set this empty to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void Ashiftjis::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void Ashiftjis::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void Ashiftjis::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
