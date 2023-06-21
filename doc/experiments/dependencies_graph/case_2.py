from module import Node, print_solution, resolve

nodes: list[Node] = [
    Node(name="A", dependencies=[]),
    Node(name="B", dependencies=["A", "D"]),
    Node(name="C", dependencies=["A", "B"]),
    Node(name="D", dependencies=["B"]),
]

print_solution(resolve(nodes))
