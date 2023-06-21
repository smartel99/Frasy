import copy


class Node:
    def __init__(self, name: str, dependencies: list[str]):
        self.name: str = name
        self.dependencies: list[str] = dependencies

    def __str__(self):
        return f"{self.name}: {self.dependencies}"

    def __repr__(self):
        return str(self)


def _has_unsolved_dependencies(target: Node, layers: list[list[Node]]) -> bool:
    if len(target.dependencies) == 0:
        return False
    target_tmp = copy.deepcopy(target)
    for layer in layers:
        for node in layer:
            if node.name in target_tmp.dependencies:
                target_tmp.dependencies.remove(node.name)
        if len(target_tmp.dependencies) == 0:
            return False
    return True


def resolve(nodes: list[Node]) -> list[list[Node]]:
    solved: list[list[Node]] = []
    while len(nodes) != 0:
        layer: list[Node] = []
        for node in nodes:
            if not _has_unsolved_dependencies(node, solved):
                layer.append(node)
        if len(layer) == 0:
            raise RuntimeError
        solved.append(layer)
        for node in layer:
            nodes.remove(node)
    return solved


def print_solution(solution: list[list[Node]]):
    for i in range(0, len(solution)):
        print("{}: {}".format(i, solution[i]))
