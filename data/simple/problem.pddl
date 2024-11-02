(define (problem move-pickup-problem)
  (:domain move-pickup)

  (:objects
    robot1 - robot
    locationA locationB locationC - location
    box1 - object
  )

  (:init
    (at robot1 locationA)
    (at-object box1 locationB)
  )

  (:goal
    (and ;; __SEQUENTIALLY
      (at-object box1 locationC)
      (at robot1 locationA)
      (at robot1 locationB)
    )
  )
)
