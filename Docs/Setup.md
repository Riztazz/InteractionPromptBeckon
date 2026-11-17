# Setup

| Part | What it is | Where it goes |
|---|---|---|
| **Channel** | The collision channel the plugin looks on | Project Settings |
| **Interactor** | Finds things to use and takes the key press | The player pawn |
| **Point** | A thing you can use, inside a sphere or box | Any actor |
| **Definition** | Instant, hold or mash, and the word on the prompt | A Data Asset |
| **Widget** | How the prompt looks | A Widget Blueprint |

How they work together, with a door as the example:

1. The **Interactor** on the player pawn looks for points near the player.
2. The door has a sphere on the channel, with a **Point** under it. The door gives the point to the Interactor
   through the **Point Provider** interface.
3. The point's **Definition**, `DA_Open`, says how it is used and what the prompt says.
4. The player presses the key. The point fires **On Interacted**, and the door opens.

---

## 1. Channel

1. Open **Project Settings > Engine > Collision**.
2. Under **Trace Channels**, click **New Trace Channel**. Name it `Interaction`. Set **Default Response** to `Ignore`.
3. Open **Project Settings > Plugins > Prompt Beckon**.
4. Set **Detect Channel** to `Interaction`.
5. Restart the editor.

**Check:** after the restart, **Detect Channel** still says `Interaction`.

**Occlude Channel** blocks the view to a point. It only matters when **Require LOS** is on. Leave it on `Visibility`.

---

## 2. Widget

1. Make a **Widget Blueprint**, such as `WBP_Prompt`.
2. Add a **Text** for the word. Add a **Progress Bar** for hold and mash.
3. Open **Class Settings**. Under **Interfaces**, add **Prompt Beckon Prompt Widget**.
4. In the graph, add the event **Set Prompt State**.
5. Break its **State** pin.
6. Connect **Verb** to **Set Text** on the Text.
7. Connect **Progress** (0 to 1) to **Set Percent** on the Progress Bar.

The plugin grows, fades and greys out the widget for you.

**Check:** the Blueprint compiles.

---

## 3. Definition

1. Make a **Data Asset** of class **Prompt Beckon Definition**, such as `DA_Open`.
2. Fill it in:

| Field | Set it to | Note |
|---|---|---|
| **Driver** | `Instant`, `Hold` or `Mash` | Hold has **Duration Seconds**. Mash has **Press Count** and **Decay Seconds** |
| **Verb** | `Open` | The word on the prompt |
| **Widget Class** | `WBP_Prompt` | From step 2 |
| **Presenter** | `Screen` or `World` | Screen draws on top of the screen. World draws in the level, so walls hide it |
| **Cooldown Seconds** | `0` | Wait time before the next use |
| **Hide On Cooldown** | on | Off shows the prompt greyed out while it waits |
| **Face Camera** | on | World prompts turn to the player |
| **Focus Look** | leave it | Size and fade of the prompt you look at, and of the rest |

Make one definition per kind of use. Use `DA_Open` on every door. Make `DA_Close` for the closed state.

**Check:** **Driver** is set.

---

## 4. Player

### Blueprint

1. Open your player pawn.
2. **Add Component > Prompt Beckon Interactor Component**.
3. Make an **Input Action** `IA_Interact` of type **Digital (bool)**. Map it to `E`.
4. In the graph, add the event **IA_Interact**.
5. Connect **Started** to **Interact Pressed** on the Interactor.
6. Connect **Completed** to **Interact Released** on the Interactor.

### C++

Add `"PromptBeckon"` to `PublicDependencyModuleNames` in your `Build.cs`.

```cpp
// MyCharacter.h
UPROPERTY(VisibleAnywhere)
TObjectPtr<UPromptBeckonInteractorComponent> Interactor;

UPROPERTY(EditAnywhere)
TObjectPtr<UInputAction> InteractAction;

// MyCharacter.cpp
#include "Interaction/InteractorComponent.h"

AMyCharacter::AMyCharacter()
{
    Interactor = CreateDefaultSubobject<UPromptBeckonInteractorComponent>(TEXT("Interactor"));
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    Input->BindAction(InteractAction, ETriggerEvent::Started, Interactor.Get(), &UPromptBeckonInteractorComponent::InteractPressed);
    Input->BindAction(InteractAction, ETriggerEvent::Completed, Interactor.Get(), &UPromptBeckonInteractorComponent::InteractReleased);
}
```

### Interactor fields

| Field | Default | What it does |
|---|---|---|
| **Detect Radius** | 250 cm | How far the player can reach |
| **Detect Offset** | 0 | Moves the reach sphere away from the pawn |
| **Stickiness** | 0.25 | Higher keeps focus on the same point, so it jumps less between close points |
| **Require LOS** | off | On: a wall between the camera and the point blocks it |
| **Scorer** | Reticle | **Reticle** picks what you look at. **Nearest** picks the closest |
| **Screen Presenter**, **World Presenter** | Screen, World | Change them only for your own presenter |

The interactor only works on a pawn that a local player controls.

**Check:** press Play. The Output Log shows no `LogPromptBeckon` warning.

---

## 5. Usable actor

### Blueprint

1. Open the actor, such as `BP_Door`.
2. **Add Component > Sphere Collision**. Size it around the part you use.
3. **Add Component > Prompt Beckon Point Component**. Drag it **under the sphere**.
4. Select the point. Set **Definition** to `DA_Open`.
5. In the **Construction Script**, call **Config Shape** with the sphere. It sets up the sphere's collision.
6. Open **Class Settings > Interfaces**. Add **Prompt Beckon Point Provider**.
7. Add the function **Get Points**. Put the point in a **Make Array** node, and return the array.
8. Select the point. Add its event **On Interacted**. Open the door there.

### C++

```cpp
// MyDoor.h
#include "Interaction/PointProvider.h"

UCLASS()
class AMyDoor : public AActor, public IPromptBeckonPointProvider
{
    GENERATED_BODY()

public:
    AMyDoor();

    virtual TArray<UPromptBeckonPointComponent*> GetPoints_Implementation() const override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Shape;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPromptBeckonPointComponent> Point;

    UFUNCTION()
    void HandleUse(UPromptBeckonPointComponent* UsedPoint, UPromptBeckonInteractorComponent* Interactor);
};

// MyDoor.cpp
#include "Components/SphereComponent.h"
#include "Interaction/PointComponent.h"

AMyDoor::AMyDoor()
{
    Shape = CreateDefaultSubobject<USphereComponent>(TEXT("Shape"));
    SetRootComponent(Shape);
    Shape->InitSphereRadius(40.f);
    UPromptBeckonPointComponent::ConfigShape(Shape);

    Point = CreateDefaultSubobject<UPromptBeckonPointComponent>(TEXT("Point"));
    Point->SetupAttachment(Shape);
}

TArray<UPromptBeckonPointComponent*> AMyDoor::GetPoints_Implementation() const
{
    return { Point.Get() };
}

void AMyDoor::BeginPlay()
{
    Super::BeginPlay();
    Point->OnInteracted.AddUniqueDynamic(this, &AMyDoor::HandleUse);
}

void AMyDoor::HandleUse(UPromptBeckonPointComponent* UsedPoint, UPromptBeckonInteractorComponent* Interactor)
{
    // Open the door here.
}
```

Set **Definition** in the actor's details, or in a Blueprint child.

To set it from C++, call `Point->SetDefinition(...)` in `PostInitializeComponents`. `BeginPlay` is too late: the
point checks its definition in its own `BeginPlay`, which runs first.

**Check:** walk up to the actor and look at it. The prompt says `Open`. Press `E`. **On Interacted** fires.

---

## Next

| To | Do this |
|---|---|
| Swap Open for Close | Call `SetDefinition(DA_Close)` in **On Interacted** |
| Lock it and show it greyed out | `SetAvail(Refused, Reason)` |
| Hide it | `SetAvail(Hidden, Reason)` |
| Unlock it | `ResetAvail()` |
| Block a press by a rule, such as "needs a key" | Add **Prompt Beckon Interaction Gate** to the actor. Return false from **Can Begin** |
| Turn off all use, such as in a menu | `Interactor->SetSuspended(true)` |
| Show an outline on focus | Use **On Focus Changed** on the point or on the interactor |
| Know how a use ended | Use **On Interact End**. Its **Result** tag says why |

### Result tags

| Tag | Means |
|---|---|
| `Interaction.Result.Completed` | The use finished |
| `Interaction.Result.Cancelled.Released` | The player let go of a hold too early |
| `Interaction.Result.Cancelled.Decayed` | The player stopped pressing during a mash |
| `Interaction.Result.Cancelled.OutOfRange` | The player walked away |
| `Interaction.Result.Cancelled.Suspended` | The interactor was turned off |
| `Interaction.Result.Cancelled.ViewerLost` | The pawn lost its player controller |
| `Interaction.Result.Aborted.TargetDestroyed` | The actor was destroyed during the use |
| `Interaction.Result.Aborted.InteractorDestroyed` | The player pawn was removed during the use |
| `Interaction.Result.Aborted.Unavailable` | The point was locked or hidden during the use |
| `Interaction.Result.Aborted.Reset` | The point was reset during the use |

You can destroy the actor inside **On Interacted**. The plugin drops its remaining events.

---

## Problems

In the **Output Log**, filter on `LogPromptBeckon`. Each line names the part that is set up wrong.

---

## Your own parts

In the definition:

- **Driver:** subclass `FPromptBeckonDriver` in C++. It shows in the **Driver** list.
- **Widget Class:** any Widget Blueprint with the **Prompt Beckon Prompt Widget** interface.

On the interactor:

- **Scorer:** subclass `FPromptBeckonFocusScorer` in C++. It shows in the **Scorer** list.
- **Screen Presenter**, **World Presenter:** subclass `UPromptBeckonPresenter` in C++.

On the usable actor:

- **Gate:** add the **Prompt Beckon Interaction Gate** interface, in Blueprint or C++.
