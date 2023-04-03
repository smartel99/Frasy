```
layers = list[list[Node]]
while unsolved is not empty:
    layer = list[Node]
    for all nodes in unsolved:
        if node dependencies are all solved:
            add node to layer
            remove node from nodes
    if layer is empty:
        error, impossible case
    add layer to layers
return layers
```


# Possible
`python .\case_1.py`
```mermaid
graph TD
A --> E
D --> E
C --> D
F --> G
A --> C
B --> C
B --> F
```


# Impossible

`python .\case_2.py`
```mermaid
graph TD
A --> B
A --> C
C --> D
D --> B
B --> C
```
