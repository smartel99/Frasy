from module import Node, print_solution, resolve

nodes: list[Node] = [
    Node(name="A", dependencies=[]),
    Node(name="B", dependencies=[]),
    Node(name="C", dependencies=["A", "B"]),
    Node(name="D", dependencies=["C"]),
    Node(name="E", dependencies=["A", "D"]),
    Node(name="F", dependencies=["B"]),
    Node(name="G", dependencies=["F"]),
]

print_solution(resolve(nodes))
